import test from 'node:test';
import assert from 'node:assert/strict';

import configs from './configs.js';

/*
 * 인증 서버 설정 스모크 테스트.
 *
 * 왜 하필 설정인가: `.env`는 gitignore돼 있어서 새로 클론한 환경에는 없다.
 * 키가 하나 빠져도 서버는 기동에 성공하고, 나중에 DB 커넥션 풀이나 Redis 접속
 * 시점에야 터진다. 그 사이의 시간이 이 티어에서 가장 비싼 디버깅이었다.
 * 여기서 먼저 걸리게 한다.
 *
 * DB·Redis에 붙지 않는다 — configs.js가 import하는 건 dotenv뿐이다.
 */

test('필수 설정 키가 모두 정의돼 있다', () => {
    const required = [
        ['port', configs.port],
        ['UserDBConfig.connectionString', configs.UserDBConfig.connectionString],
        ['UserDBConfig.driver', configs.UserDBConfig.driver],
        ['redisHost', configs.redisHost],
        ['redisPort', configs.redisPort],
        ['accessTokenTTL', configs.accessTokenTTL],
    ];

    for (const [name, value] of required) {
        assert.ok(value !== undefined && value !== '', `.env에 ${name}에 해당하는 값이 없다`);
    }
});

test('DB 커넥션 풀 크기가 숫자로 파싱된다', () => {
    // parseInt(undefined)는 예외가 아니라 NaN이다. 그대로 mssql에 넘어가면
    // 풀 설정이 조용히 무의미해진다.
    const { pool } = configs.UserDBConfig;

    assert.ok(Number.isInteger(pool.max), 'DB_MAX_CONNECTION이 정수로 파싱되지 않았다');
    assert.ok(Number.isInteger(pool.min), 'DB_MIN_CONNECTION이 정수로 파싱되지 않았다');
    assert.ok(Number.isInteger(pool.idleTimeoutMillis), 'DB_IDLE_TIMEOUT이 정수로 파싱되지 않았다');

    assert.ok(pool.min <= pool.max, `풀 최솟값(${pool.min})이 최댓값(${pool.max})보다 크다`);
});
