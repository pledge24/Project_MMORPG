import sql from 'mssql/msnodesqlv8.js';

/** 계정 번호. AccountDB의 BIGINT를 10진수 문자열로 다룬다. 계정 번호로 계산하지 않으므로 숫자로 바꾸지 않는다. */
export type AccountId = string;

export interface NewAccount {
    /** 소문자로 바꾸고 형식 검사를 통과한 값 */
    loginId: string;
    passwordHash: string;
}

export type CreateAccountResult =
    | { kind: 'created'; accountId: AccountId }
    | { kind: 'duplicate' };

/** 로그인 판정에 쓰는 계정 정보 */
export interface LoginAccount {
    accountId: AccountId;
    passwordHash: string;
    /** null이면 정상이다. 값이 있으면 그 시각까지 로그인을 거절한다. */
    bannedUntil: Date | null;
}

/**
 * 계정을 저장하는 곳.
 *
 * 회원가입과 로그인의 판정은 이 인터페이스만 안다. DB를 아는 코드는 구현뿐이다.
 */
export interface AccountRepository {
    /** 같은 로그인 ID의 계정이 이미 있으면 만들지 않고 duplicate를 돌려준다. */
    create(account: NewAccount): Promise<CreateAccountResult>;

    /** 소문자로 바꾼 로그인 ID로 찾는다. 없으면 undefined다. */
    findByLoginId(loginId: string): Promise<LoginAccount | undefined>;

    /** 마지막 로그인 시각을 지금으로 바꾼다. */
    recordLogin(accountId: AccountId): Promise<void>;
}

// UNIQUE 제약 위반(2627)과 고유 인덱스 위반(2601). Account.LoginId에는 UNIQUE 제약이 걸려 있다.
const DUPLICATE_KEY_ERROR_NUMBERS = new Set([2627, 2601]);

/** AccountDB의 Account 테이블에 계정을 두는 저장소. 연결된 풀을 받는다. 시각 열은 모두 UTC다. */
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

    async findByLoginId(loginId: string): Promise<LoginAccount | undefined> {
        // BannedUntil을 ISO 8601 문자열로 받는다. Date로 받으면 드라이버가 시간대를 어떻게 해석하는지에 판정이 걸린다.
        const result = await this.pool.request()
            .input('loginId', sql.VarChar(32), loginId)
            .query<{ AccountId: string; PasswordHash: string; BannedUntil: string | null }>(`
                SELECT AccountId, PasswordHash, CONVERT(VARCHAR(23), BannedUntil, 126) AS BannedUntil
                FROM dbo.Account
                WHERE LoginId = @loginId
            `);

        const row = result.recordset[0];
        if (row === undefined) {
            return undefined;
        }
        return {
            accountId: String(row.AccountId),
            passwordHash: row.PasswordHash,
            bannedUntil: row.BannedUntil === null ? null : new Date(`${row.BannedUntil}Z`),
        };
    }

    async recordLogin(accountId: AccountId): Promise<void> {
        // 계정 번호는 2^53을 넘을 수 있어 숫자로 바꾸지 않고 문자열로 넘겨 SQL에서 BIGINT로 바꾼다.
        await this.pool.request()
            .input('accountId', sql.VarChar(20), accountId)
            .query(`
                UPDATE dbo.Account
                SET LastLoginAt = SYSUTCDATETIME()
                WHERE AccountId = CAST(@accountId AS BIGINT)
            `);
    }
}
