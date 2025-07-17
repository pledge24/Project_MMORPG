import express from 'express';
import cookieParser from 'cookie-parser';
import LoginRouter from './routes/login.router.js';
import AccountRouter from './routes/account.router.js';
import RedisClient from './DB/redis.js';
import configs from './Config/configs.js';
import connectionPool from '../src/DB/connectPool.js';

const app = express();
const PORT = configs.port;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(cookieParser());
app.use([router, LoginRouter]);
app.use('/Account', AccountRouter);

// Init DB.
RedisClient.connect();
connectionPool;

app.get('/', (req, res) => {
    return res.json({ message: 'Welcome To AuthServer' });
});

app.listen(PORT, () => {
    console.log(PORT, '포트로 서버가 열림!');
});
