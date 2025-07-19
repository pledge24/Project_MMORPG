import express from 'express';
import sql from 'mssql/msnodesqlv8.js';
import pool from "../DB/connectPool.js";
import bcrypt from 'bcrypt';

const router = express.Router();

/** 회원가입 API */
router.post('/Register', async (req, res) => {
    try {
        const { username, password } = req.body;

        // 1. 유효성 검사
        if (!username) {
            return res.status(400).json({ errorMessage: '로그인할 아이디를 입력하여 주세요.' });
        }

        if (!password) {
            return res.status(400).json({ errorMessage: '비밀번호를 입력하여 주세요.' });
        }

        if (!/^[a-zA-Z0-9]{4,10}$/.test(username)) {
            return res.status(400).json({ errorMessage: '아이디 조건: 영문자, 숫자로 이루어진 4~10 길의 문자열' });
        }

        if (!/^[a-zA-Z0-9]{4,20}$/.test(password)) {
            return res.status(400).json({ errorMessage: '비밀번호 조건 : 영문자, 숫자로 이루어진 4~20 길이의 문자열' });
        }

        // 2. 아이디 중복 확인.
        const existing = await pool.request()
            .input('username', sql.VarChar, username)
            .query(`
                SELECT *
                FROM dbo.Users
                WHERE username = @username
            `);

        if (existing.recordset.length > 0) {
            return res.status(409).json({ errorMessage: '이미 존재하는 아이디입니다.' });
        }
            
                
        // 3. UserDB에 사용자 추가
        const password_hash = await bcrypt.hash(password, 10);

        await pool.request()
            .input('username', sql.VarChar, username)
            .input('password_hash', sql.VarChar, password_hash)
            .input('created_at', sql.DateTime, new Date())
            .query(`
                INSERT INTO dbo.Users (username, password_hash, created_at)
                VALUES (@username, @password_hash, @created_at)
            `);

        return res.status(201).json({ message: '회원가입 완료' });
    }
    catch (err) {
        console.log(`회원가입 오류 발생: ${err}`);
        return res.status(500).json({ errorMessage: 'Internal Error' });
    }
});

export default router;
