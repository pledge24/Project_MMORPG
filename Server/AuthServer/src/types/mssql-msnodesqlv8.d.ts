// mssql은 타입 정의를 싣지 않는다. @types/mssql은 쓰지 않는 드라이버 tedious를 끌어오므로, 이 서버가 쓰는 API만 선언한다.
// mssql을 올리면 여기 적은 시그니처가 바뀌지 않았는지 확인한다.
declare module 'mssql/msnodesqlv8.js' {
    namespace sql {
        interface ISqlType {
            readonly type: unknown;
        }

        interface ConnectionConfig {
            connectionString: string;
            pool?: { max?: number; min?: number; idleTimeoutMillis?: number };
            options?: Record<string, unknown>;
        }

        interface IResult<T> {
            recordset: T[];
            rowsAffected: number[];
        }

        class Request {
            input(name: string, type: ISqlType, value: unknown): this;
            query<T = Record<string, unknown>>(command: string): Promise<IResult<T>>;
        }

        class ConnectionPool {
            constructor(config: ConnectionConfig);
            connect(): Promise<ConnectionPool>;
            close(): Promise<void>;
            request(): Request;
        }

        /** 쿼리가 실패하면 던진다. number는 SQL Server의 오류 번호다. */
        class RequestError extends Error {
            readonly code: string;
            readonly number?: number;
        }

        function VarChar(length?: number): ISqlType;
    }

    export default sql;
}
