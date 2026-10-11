import redis from 'redis';
import configs from '../Config/configs.js';

// Redis 클라이언트 생성. node-redis v5는 주소를 socket 아래에서만 읽는다. 최상위 host와 port는 무시된다.
const redisClient = redis.createClient({
    socket: {
        host: configs.redisHost,
        port: Number(configs.redisPort)
    }
});

redisClient.on('connect', () => {
    console.log('Redis 클라이언트가 연결되었습니다.');
});

redisClient.on('disconnect', () => {
    console.log('Redis 클라이언트가 종료되었습니다.');
});

redisClient.on('error', (err) => {
    console.error('Redis 오류:', err);
});

export default redisClient;
