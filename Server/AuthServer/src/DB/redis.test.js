import test from 'node:test';
import assert from 'node:assert/strict';

import configs from '../Config/configs.js';
import redisClient from './redis.js';

/*
 * Redis 클라이언트 설정 테스트.
 *
 * node-redis v5의 옵션에는 최상위 host와 port가 없고 socket: { host, port }만 있다. 최상위에 넘긴 값은 무시되어
 * 클라이언트가 기본값(localhost:6379)으로 붙었다(TD-005). 지금은 설정값과 기본값이 같아서 드러나지 않는다.
 *
 * Redis에 붙지 않는다. connect()는 app.js가 부르고, 여기서는 만든 클라이언트의 옵션만 읽는다.
 */

test('Redis 주소 설정이 클라이언트의 소켓 옵션으로 넘어간다', () => {
    const socket = redisClient.options?.socket;

    assert.ok(socket !== undefined, 'socket 옵션이 없다. 최상위 host와 port는 node-redis가 무시한다');
    assert.equal(socket.host, configs.redisHost);
    assert.equal(socket.port, Number(configs.redisPort), '포트는 숫자로 넘긴다');
});
