import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { parseEnv } from 'node:util';
import { describe, expect, test } from 'vitest';

import { ConfigError, loadConfig, REQUIRED_ENV_KEYS } from './config.js';

// .env는 커밋하지 않으므로, 새로 클론한 환경은 예시 파일을 복사해 시작한다. 두 목록이 어긋나면 기동하자마자 설정 검사에서 멈춘다.
const EXAMPLE_ENV_PATH = join(import.meta.dirname, '..', '..', '.env.example');

const VALID_ENV = {
    PORT: '5000',
    DB_CONNECTION_STRING: 'Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\P1Auth;Database=AccountDB;Trusted_Connection=yes;',
    DB_MAX_CONNECTION: '5',
    DB_MIN_CONNECTION: '1',
    DB_IDLE_TIMEOUT: '30000',
    REDIS_HOST: '127.0.0.1',
    REDIS_PORT: '6379',
    TICKET_TTL_SECONDS: '30',
};

function problemsOf(env: Record<string, string | undefined>): string[] {
    try {
        loadConfig(env);
    }
    catch (err) {
        if (err instanceof ConfigError) {
            return err.problems;
        }
        throw err;
    }
    return [];
}

describe('loadConfig', () => {
    test('모든 키가 형식에 맞으면 숫자로 바꾼 설정을 돌려준다', () => {
        const config = loadConfig(VALID_ENV);

        expect(config.port).toBe(5000);
        expect(config.accountDb.connectionString).toBe(VALID_ENV.DB_CONNECTION_STRING);
        expect(config.accountDb.pool).toEqual({ max: 5, min: 1, idleTimeoutMillis: 30000 });
        expect(config.redis).toEqual({ host: '127.0.0.1', port: 6379 });
        expect(config.ticketTtlSeconds).toBe(30);
    });

    test('빠지거나 빈 키를 모두 모아 알린다', () => {
        const problems = problemsOf({ ...VALID_ENV, PORT: undefined, REDIS_HOST: '' });

        expect(problems).toHaveLength(2);
        expect(problems.join('\n')).toContain('PORT');
        expect(problems.join('\n')).toContain('REDIS_HOST');
    });

    test.each([
        ['PORT', 'abc'],
        ['PORT', '70000'],
        ['DB_MAX_CONNECTION', '5개'],
        ['DB_IDLE_TIMEOUT', '-1'],
        ['REDIS_PORT', '0'],
        ['TICKET_TTL_SECONDS', '1.5'],
    ])('%s=%s는 형식 오류다', (key, value) => {
        const problems = problemsOf({ ...VALID_ENV, [key]: value });

        expect(problems).toHaveLength(1);
        expect(problems[0]).toContain(key);
    });

    test('풀 최솟값이 최댓값보다 크면 형식 오류다', () => {
        const problems = problemsOf({ ...VALID_ENV, DB_MIN_CONNECTION: '6', DB_MAX_CONNECTION: '5' });

        expect(problems).toHaveLength(1);
        expect(problems[0]).toContain('DB_MIN_CONNECTION');
    });
});

describe('예시 환경 파일', () => {
    // Node가 --env-file을 읽는 것과 같은 규칙으로 읽는다.
    const exampleEnv = parseEnv(readFileSync(EXAMPLE_ENV_PATH, 'utf8'));

    test('키가 설정 모듈이 요구하는 키와 같다', () => {
        expect(Object.keys(exampleEnv).sort()).toEqual([...REQUIRED_ENV_KEYS].sort());
    });

    test('값이 설정 검사를 통과한다', () => {
        expect(problemsOf(exampleEnv)).toEqual([]);
    });
});
