import sql from 'mssql/msnodesqlv8.js';

import type { AccountRepository, CreateAccountResult, NewAccount } from './accountRepository.js';

// UNIQUE 제약 위반(2627)과 고유 인덱스 위반(2601). Account.LoginId에는 UNIQUE 제약이 걸려 있다.
const DUPLICATE_KEY_ERROR_NUMBERS = new Set([2627, 2601]);

/** AccountDB의 Account 테이블에 계정을 두는 저장소. 연결된 풀을 받는다. */
export class MssqlAccountRepository implements AccountRepository {
    private readonly pool: sql.ConnectionPool;

    constructor(pool: sql.ConnectionPool) {
        this.pool = pool;
    }

    async create(account: NewAccount): Promise<CreateAccountResult> {
        try {
            const result = await this.pool.request()
                .input('loginId', sql.VarChar(32), account.loginId)
                .input('passwordHash', sql.VarChar(255), account.passwordHash)
                .query<{ AccountId: string }>(`
                    INSERT INTO dbo.Account (LoginId, PasswordHash)
                    OUTPUT INSERTED.AccountId
                    VALUES (@loginId, @passwordHash)
                `);

            // 드라이버는 BIGINT를 문자열로 돌려준다. 드라이버가 바뀌어도 문자열이 되도록 한 번 더 바꾼다.
            return { kind: 'created', accountId: String(result.recordset[0].AccountId) };
        }
        catch (err) {
            if (err instanceof sql.RequestError && err.number !== undefined && DUPLICATE_KEY_ERROR_NUMBERS.has(Number(err.number))) {
                return { kind: 'duplicate' };
            }
            throw err;
        }
    }
}
