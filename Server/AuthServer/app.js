import express from 'express'
import redis from 'redis'

const app = express();
const PORT = 5000;

app.get('/', (req, res) => {
    res.send('Hello World!');
});

app.listen(PORT, () => {
    console.log(PORT, '포트로 서버가 열림!');
});

// Redis 클라이언트 생성
const client = redis.createClient({
    host: 'localhost',  // Redis 서버 주소
    port: 6379,         // Redis 기본 포트
    // password: 'your_password', // 비밀번호가 있는 경우
});

// 연결 이벤트 처리
client.on('connect', () => {
    console.log('Redis 클라이언트가 연결되었습니다.');
});

client.on('error', (err) => {
    console.error('Redis 연결 오류:', err);
});

// Redis 연결
async function connectRedis() {
    try {
        await client.connect();
        console.log('Redis에 성공적으로 연결되었습니다.');
    } catch (error) {
        console.error('Redis 연결 실패:', error);
    }
}

// 기본 Redis 작업 예제
async function redisOperations() {
    try {
        // 문자열 저장
        await client.set('name', '홍길동');
        await client.set('age', '30');

        // 문자열 조회
        const name = await client.get('name');
        const age = await client.get('age');
        console.log(`이름: ${name}, 나이: ${age}`);

        // 만료 시간 설정 (10초)
        await client.setEx('temp_key', 10, '임시 데이터');

        // 해시 저장
        await client.hSet('user:1001', {
            name: '김철수',
            email: 'kim@example.com',
            age: 25
        });

        // 해시 조회
        const user = await client.hGetAll('user:1001');
        console.log('사용자 정보:', user);

        // 리스트 작업
        await client.lPush('tasks', ['할일1', '할일2', '할일3']);
        const tasks = await client.lRange('tasks', 0, -1);
        console.log('할일 목록:', tasks);

        // 집합(Set) 작업
        await client.sAdd('colors', ['빨강', '파랑', '노랑']);
        const colors = await client.sMembers('colors');
        console.log('색상 집합:', colors);

        // 키 존재 확인
        const exists = await client.exists('name');
        console.log('name 키 존재:', exists === 1);

        // 키 삭제
        await client.del('temp_key');

        // TTL 확인 (Time To Live)
        const ttl = await client.ttl('name');
        console.log('name 키의 TTL:', ttl);

    } catch (error) {
        console.error('Redis 작업 오류:', error);
    }
}

// 연결 종료
async function closeConnection() {
    try {
        await client.disconnect();
        console.log('Redis 연결이 종료되었습니다.');
    } catch (error) {
        console.error('연결 종료 오류:', error);
    }
}

// 실행
async function main() {
    await connectRedis();
    await redisOperations();
    await closeConnection();
}

main().catch(console.error);

//--------------------
app.use(express.json());

// API 엔드포인트 예제
app.get('/user/:id', async (req, res) => {
    try {
        const userId = req.params.id;
        const user = await client.hGetAll(`user:${userId}`);

        if (Object.keys(user).length === 0) {
            return res.status(404).json({ error: '사용자를 찾을 수 없습니다.' });
        }

        res.json(user);
    } catch (error) {
        res.status(500).json({ error: 'Redis 오류' });
    }
});

app.post('/user', async (req, res) => {
    try {
        const { id, name, email, age } = req.body;

        await client.hSet(`user:${id}`, {
            name,
            email,
            age: age.toString()
        });

        res.json({ message: '사용자가 저장되었습니다.' });
    } catch (error) {
        res.status(500).json({ error: 'Redis 저장 오류' });
    }
});

// 캐시 미들웨어 예제
const cacheMiddleware = (duration = 300) => {
    return async (req, res, next) => {
        const key = `cache:${req.originalUrl}`;

        try {
            const cached = await client.get(key);

            if (cached) {
                return res.json(JSON.parse(cached));
            }

            // 원래 res.json을 저장
            const originalJson = res.json;

            // res.json을 오버라이드하여 캐시 저장
            res.json = function (data) {
                client.setEx(key, duration, JSON.stringify(data));
                return originalJson.call(this, data);
            };

            next();
        } catch (error) {
            next();
        }
    };
};

// 캐시 미들웨어 사용 예제
app.get('/expensive-operation', cacheMiddleware(600), async (req, res) => {
    // 시간이 오래 걸리는 작업 시뮬레이션
    await new Promise(resolve => setTimeout(resolve, 2000));

    res.json({
        message: '비용이 많이 드는 작업 결과',
        timestamp: new Date().toISOString()
    });
});