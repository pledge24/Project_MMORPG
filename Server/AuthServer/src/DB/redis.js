import redis from 'redis';
import configs from '../Config/configs.js';

// Redis 클라이언트 생성
const redisClient = redis.createClient({
    host: configs.redisHost,
    port: configs.redisPort
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
