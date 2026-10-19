import sql from 'mssql/msnodesqlv8.js';

import type { Config } from './config.js';

/** AccountDB 커넥션 풀을 만들고 연결한다. 연결에 실패하면 그 에러로 끝난다. */
export async function connectAccountDb({ connectionString, pool }: Config['accountDb']): Promise<sql.ConnectionPool> {
    const accountDbPool = new sql.ConnectionPool({
        connectionString,
        pool,
        options: {
            encrypt: false,
            trustServerCertificate: true,
        },
    });
    await accountDbPool.connect();
    return accountDbPool;
}
