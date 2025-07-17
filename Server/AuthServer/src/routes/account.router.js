import express from 'express';
import configs from '../Config/configs.js';

const router = express.router();

/** 회원가입 API */
router.post('/Register', async (req, res) => {
    try {
        const { username, password } = req.body;

        if (!username) {
            return res.status(401).json({ errorMessage: '로그인할 아이디를 입력하여 주세요.' });
        }

        if (!password) {
            return res.status(401).json({ errorMessage: '비밀번호를 입력하여 주세요.' });
        }

        // 아이디 중복 확인.

        // UserDB에 유저 추가

        return res.status(201).json({ message: '회원가입 완료' });
    }
    catch (err) {
        return res.status(500).json({ errorMessage: 'Internal Error' });
    }
});

export default router;
