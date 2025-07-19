import express from 'express';
import bcrypt from 'bcrypt';
import redisClient from '../DB/redis.js';
import { v4 as uuidv4 } from 'uuid';
import configs from '../Config/configs.js';
import sql from 'mssql/msnodesqlv8.js';
import pool from '../DB/connectPool.js';

const router = express.Router();

/** 로그인 API */
router.post('/Login', async (req, res) => {
    try {
        const { username, password } = req.body;

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
                SELECT *
                FROM dbo.Users 
                WHERE username = @username
            `);

        if (result.recordset.length === 0) {
            return res.status(404).json({ errorMessage: '존재하지 않는 계정입니다.' });
        }

        const account = result.recordset[0];

        // 비밀번호 암호화한 다음 DB랑 대조
        const isMatch = await bcrypt.compare(password, account.password_hash);
        if (!isMatch) {
            return res.status(400).json({ errorMessage: '비밀번호가 일치하지 않습니다.' });
        }

        // 최근 로그인 시간 갱신
        await pool.request()
            .input('username', sql.VarChar, username)
            .query(`
                UPDATE dbo.Users
                SET last_login = GETDATE()
                WHERE username = @username
            `);

        // Access Token 정책: UUID + Redis/DB 세션 저장 구조
        const token = uuidv4();

        await redisClient.set(`accessToken:${token}`, JSON.stringify({ username }), {
            EX: configs.accessTokenTTL
        });

        return res.status(200).json({ accessToken: token });
    }
    catch (err) {
        return res.status(500).json({ errorMessage: 'Internal Error' });
    };
});

export default router;