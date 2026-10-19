import sql from 'mssql/msnodesqlv8.js';

import { MssqlAccountRepository } from './accounts/mssqlAccountRepository.js';
import { createApp } from './app.js';
import { LoginService } from './auth/loginService.js';
import { RegisterService } from './auth/registerService.js';
import { loadConfig } from './config/config.js';
import { createRedisClient, RedisTicketRepository } from './tickets/redisTicketRepository.js';

// 설정 검사, AccountDB 연결, Redis 연결을 이 순서로 모두 확인한 뒤에 포트를 연다. 연결에 실패한 채 포트를 열면 서버가 떠 있는 것처럼 보이지만 모든 로그인이 500으로 실패한다.
async function main(): Promise<void> {
    const config = loadConfig(process.env);

    const accountDbPool = new sql.ConnectionPool({
        connectionString: config.accountDb.connectionString,
        pool: config.accountDb.pool,
        options: {
            encrypt: false,
            trustServerCertificate: true,
        },
    });
    await accountDbPool.connect();
    console.log('AccountDB 연결 성공');

    const redisClient = createRedisClient(config.redis);
    await redisClient.connect();
    console.log('Redis 연결 성공');

    const accounts = new MssqlAccountRepository(accountDbPool);
    const app = createApp({
        registerService: new RegisterService(accounts),
        loginService: new LoginService(accounts, new RedisTicketRepository(redisClient, config.ticketTtlSeconds)),
    });

    // Express는 포트를 열지 못한 에러(포트 사용 중 등)도 이 콜백으로 넘긴다. 에러를 보지 않으면 실패한 채 떠 있는 것처럼 보인다.
    await new Promise<void>((resolve, reject) => {
        app.listen(config.port, (err?: Error) => (err ? reject(err) : resolve()));
    });
    console.log(`인증 서버가 ${config.port} 포트에서 접속을 받는다`);
}

main().catch((err: unknown) => {
    console.error('인증 서버 기동 실패:', err);
    process.exit(1);
});
