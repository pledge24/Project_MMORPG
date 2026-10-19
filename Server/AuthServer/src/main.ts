import sql from 'mssql/msnodesqlv8.js';

import { MssqlAccountRepository } from './accounts/mssqlAccountRepository.js';
import { createApp } from './app.js';
import { RegisterService } from './auth/registerService.js';
import { loadConfig } from './config/config.js';
import legacyPool from './legacy/connectPool.js';
import legacyLoginRouter from './legacy/login.router.js';
import legacyRedisClient from './legacy/redis.js';

// 설정 검사와 DB 연결을 모두 확인한 뒤에 포트를 연다. 연결에 실패한 채 포트를 열면 서버가 떠 있는 것처럼 보이지만 모든 요청이 500으로 실패한다.
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

    // 옛 로그인 경로는 자기 풀과 Redis 클라이언트를 쓴다. Redis 연결은 옛 진입점처럼 기다리지 않는다.
    await legacyPool.connect();
    legacyRedisClient.connect().catch((err: unknown) => {
        console.error('Redis 연결 실패:', err);
    });

    const app = createApp({
        registerService: new RegisterService(new MssqlAccountRepository(accountDbPool)),
        legacyRouter: legacyLoginRouter,
    });

    app.listen(config.port, () => {
        console.log(`인증 서버가 ${config.port} 포트에서 접속을 받는다`);
    });
}

main().catch((err: unknown) => {
    console.error('인증 서버 기동 실패:', err);
    process.exit(1);
});
