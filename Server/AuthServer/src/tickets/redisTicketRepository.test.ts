import { expect, test } from 'vitest';

import { createRedisClient } from './redisTicketRepository.js';

// Redis에 붙지 않는다. 만든 클라이언트의 옵션만 읽는다.
test('Redis 주소 설정이 클라이언트의 소켓 옵션으로 넘어간다', () => {
    const client = createRedisClient({ host: '10.0.0.1', port: 6390 });

    // 최상위 host와 port는 node-redis 5가 무시한다. 소켓 옵션에 있어야 그 주소로 붙는다.
    expect(client.options?.socket).toMatchObject({ host: '10.0.0.1', port: 6390 });
});
