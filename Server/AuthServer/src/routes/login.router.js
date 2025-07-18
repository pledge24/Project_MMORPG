import express from "express";
import bcrypt from 'bcrypt';
import redisClient from "../DB/redis.js";
import { v4 as uuidv4 } from "uuid";
import configs from '../Config/configs.js';

const router = express.Router();

/** 로그인 API */
router.post("/Login", async (req, res) => {
    try {
        const { username, password } = req.body;

        if (!username) {
            return res.status(401).json({ errorMessage: '로그인할 아이디를 입력하여 주세요.' });
        }

        if (!password) {
            return res.status(401).json({ errorMessage: '비밀번호를 입력하여 주세요.' });
        }

        // 유저 정보 가져온다.
        //const account =

        if (!account) {
            return res.status(404).json({ errorMessage: '존재하지 않는 아이디입니다.' });
        }

        // 비밀번호 암호화한 다음 DB랑 대조
        if (!(await bcrypt.compare(password, account.password))) {
            return res.status(401).json({ errorMessage: '비밀번호가 일치하지 않습니다.' });
        }

        // Access Token 정책: UUID + Redis/DB 세션 저장 구조
        const token = uuidv4();

        await redisClient.set('access_token:${token}', JSON.stringify({ username }), {
            EX: configs.accessTokenTTL
        });

        return res.json({ accessToken: token });
    }
    catch (err) {
        return res.status(500).json({ errorMessage: 'Internal Error' });
    };
});

export default router;


//// API 엔드포인트 예제
//router.get('/user/:id', async (req, res) => {
//    try {
//        const userId = req.params.id;
//        const user = await client.hGetAll(`user:${userId}`);

//        if (Object.keys(user).length === 0) {
//            return res.status(404).json({ error: '사용자를 찾을 수 없습니다.' });
//        }

//        res.json(user);
//    } catch (error) {
//        res.status(500).json({ error: 'Redis 오류' });
//    }
//});

//router.post('/user', async (req, res) => {
//    try {
//        const { id, name, email, age } = req.body;

//        await client.hSet(`user:${id}`, {
//            name,
//            email,
//            age: age.toString()
//        });

//        res.json({ message: '사용자가 저장되었습니다.' });
//    } catch (error) {
//        res.status(500).json({ error: 'Redis 저장 오류' });
//    }
//});

//// 캐시 미들웨어 예제
//const cacheMiddleware = (duration = 300) => {
//    return async (req, res, next) => {
//        const key = `cache:${req.originalUrl}`;

//        try {
//            const cached = await client.get(key);

//            if (cached) {
//                return res.json(JSON.parse(cached));
//            }

//            // 원래 res.json을 저장
//            const originalJson = res.json;

//            // res.json을 오버라이드하여 캐시 저장
//            res.json = function (data) {
//                client.setEx(key, duration, JSON.stringify(data));
//                return originalJson.call(this, data);
//            };

//            next();
//        } catch (error) {
//            next();
//        }
//    };
//};

//// 캐시 미들웨어 사용 예제
//app.get('/expensive-operation', cacheMiddleware(600), async (req, res) => {
//    // 시간이 오래 걸리는 작업 시뮬레이션
//    await new Promise(resolve => setTimeout(resolve, 2000));

//    res.json({
//        message: '비용이 많이 드는 작업 결과',
//        timestamp: new Date().toISOString()
//    });
//});



//// 기본 Redis 작업 예제
//async function redisOperations() {
//    try {
//        // 문자열 저장
//        await client.set('name', '홍길동');
//        await client.set('age', '30');

//        // 문자열 조회
//        const name = await client.get('name');
//        const age = await client.get('age');
//        console.log(`이름: ${name}, 나이: ${age}`);

//        // 만료 시간 설정 (10초)
//        await client.setEx('temp_key', 10, '임시 데이터');

//        // 해시 저장
//        await client.hSet('user:1001', {
//            name: '김철수',
//            email: 'kim@example.com',
//            age: 25
//        });

//        // 해시 조회
//        const user = await client.hGetAll('user:1001');
//        console.log('사용자 정보:', user);

//        // 리스트 작업
//        await client.lPush('tasks', ['할일1', '할일2', '할일3']);
//        const tasks = await client.lRange('tasks', 0, -1);
//        console.log('할일 목록:', tasks);

//        // 집합(Set) 작업
//        await client.sAdd('colors', ['빨강', '파랑', '노랑']);
//        const colors = await client.sMembers('colors');
//        console.log('색상 집합:', colors);

//        // 키 존재 확인
//        const exists = await client.exists('name');
//        console.log('name 키 존재:', exists === 1);

//        // 키 삭제
//        await client.del('temp_key');

//        // TTL 확인 (Time To Live)
//        const ttl = await client.ttl('name');
//        console.log('name 키의 TTL:', ttl);

//    } catch (error) {
//        console.error('Redis 작업 오류:', error);
//    }
//}


