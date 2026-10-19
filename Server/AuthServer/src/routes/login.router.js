import express from 'express';
import bcrypt from 'bcrypt';
import redisClient from '../DB/redis.js';
import { randomUUID } from 'node:crypto';
import configs from '../Config/configs.js';
import sql from 'mssql/msnodesqlv8.js';
import pool from '../DB/connectPool.js';

const router = express.Router();

/** 로그인 API */
router.post('/Login', async (req, res) => {
    try {
        // Express 5는 본문이 없는 요청의 req.body를 undefined로 둔다. 구조 분해가 예외로 500을 내지 않게 한다.
        const { username, password } = req.body ?? {};

        if (!username) {
            return res.status(400).json({ errorMessage: '로그인할 아이디를 입력하여 주세요.' });
        }

        if (!password) {
            return res.status(400).json({ errorMessage: '비밀번호를 입력하여 주세요.' });
        }

        // 유저 정보를 가져온다.
        const result = await pool.request()
            .input('username', sql.VarChar, username)
            .query(`
                SELECT AccountId, PasswordHash
                FROM dbo.Account
                WHERE LoginId = @username
            `);

        if (result.recordset.length === 0) {
            return res.status(404).json({ errorMessage: '존재하지 않는 계정입니다.' });
        }

        const account = result.recordset[0];

        // 비밀번호 암호화한 다음 DB랑 대조
        const isMatch = await bcrypt.compare(password, account.PasswordHash);
        if (!isMatch) {
            return res.status(400).json({ errorMessage: '비밀번호가 일치하지 않습니다.' });
        }

        // 최근 로그인 시간 갱신
        await pool.request()
            .input('username', sql.VarChar, username)
            .query(`
                UPDATE dbo.Account
                SET LastLoginAt = SYSUTCDATETIME()
                WHERE LoginId = @username
            `);

        // Access Token 정책: UUID + Redis/DB 세션 저장 구조
        const token = randomUUID();
        // mssql 드라이버는 AccountId(BIGINT)를 문자열로 돌려준다. 게임 서버는 userId를 JSON 정수로만 받으므로 숫자로 바꾼다.
        const userId = Number(account.AccountId);

        await redisClient.set(`accessToken:${token}`, JSON.stringify({ userId, username }), {
            EX: configs.accessTokenTTL
        });

        return res.status(200).json({ accessToken: token });
    }
    catch (err) {
        console.error('로그인 처리 중 오류:', err);
        return res.status(500).json({ errorMessage: 'Internal Error' });
    };
});

export default router;