import sql from 'mssql/msnodesqlv8.js';

/** 월드 선택이 다시 확인하는 계정 상태 */
export interface AccountStatus {
    /** null이면 정상이다. 값이 있으면 그 시각까지 월드 선택을 거절한다. */
    bannedUntil: Date | null;
}

/**
 * 월드 선택이 계정 상태를 읽는 곳.
 *
 * 월드 선택의 판정은 이 인터페이스만 안다. DB를 아는 코드는 구현뿐이다.
 * 계정 번호는 10진수 문자열로 받는다. auth 도메인의 타입을 import하지 않도록 문자열로 둔다.
 */
export interface AccountStatusRepository {
    /** 계정이 없으면 undefined다. */
    findStatus(accountId: string): Promise<AccountStatus | undefined>;
}

/** AccountDB의 Account 테이블에서 계정 상태를 읽는 저장소. 연결된 풀을 받는다. 시각 열은 모두 UTC다. */
export class MssqlAccountStatusRepository implements AccountStatusRepository {
    private readonly pool: sql.ConnectionPool;

    constructor(pool: sql.ConnectionPool) {
        this.pool = pool;
    }

    async findStatus(accountId: string): Promise<AccountStatus | undefined> {
        // 계정 번호는 2^53을 넘을 수 있어 숫자로 바꾸지 않고 문자열로 넘겨 SQL에서 BIGINT로 바꾼다.
        // BannedUntil을 ISO 8601 문자열로 받는다. Date로 받으면 드라이버가 시간대를 어떻게 해석하는지에 판정이 걸린다.
        const result = await this.pool.request()
            .input('accountId', sql.VarChar(20), accountId)
            .query<{ BannedUntil: string | null }>(`
                SELECT CONVERT(VARCHAR(23), BannedUntil, 126) AS BannedUntil
                FROM dbo.Account
                WHERE AccountId = CAST(@accountId AS BIGINT)
            `);

        const row = result.recordset[0];
        if (row === undefined) {
            return undefined;
        }
        return { bannedUntil: row.BannedUntil === null ? null : new Date(`${row.BannedUntil}Z`) };
    }
}
