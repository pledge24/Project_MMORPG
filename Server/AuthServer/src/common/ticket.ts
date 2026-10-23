import { createClient } from 'redis';

import type { Config } from './config.js';

/** 인증 티켓. crypto.randomUUID()가 만든 UUID 문자열이다. */
export type Ticket = string;

/**
 * 인증 티켓을 두는 곳.
 *
 * 판정 코드는 이 인터페이스만 안다. Redis를 아는 코드는 구현뿐이다. 소비는 게임 서버가 하므로 여기에 없다.
 * 계정 번호는 10진수 문자열로 받는다. 공용 코드가 도메인의 타입을 import하지 않도록 문자열로 둔다.
 */
export interface TicketRepository {
    /** worldId 월드에서만 쓸 수 있는 티켓이 가리키는 계정 번호를 저장한다. 유효 시간은 구현이 정한다. */
    save(worldId: number, ticket: Ticket, accountId: string): Promise<void>;
}

export type RedisClient = ReturnType<typeof createClient>;

// 키 이름은 게임 서버와의 계약이다. 바꾸면 게임 서버가 티켓을 찾지 못한다.
const TICKET_KEY_PREFIX = 'authTicket:';

/**
 * 인증 티켓을 Redis의 키 하나로 두는 저장소. 값은 계정 번호 문자열 하나이고 TTL이 붙는다.
 * 키에 월드 번호가 들어가므로, 그 월드의 게임 서버만 티켓을 찾는다.
 */
export class RedisTicketRepository implements TicketRepository {
    private readonly client: RedisClient;
    private readonly ttlSeconds: number;

    constructor(client: RedisClient, ttlSeconds: number) {
        this.client = client;
        this.ttlSeconds = ttlSeconds;
    }

    async save(worldId: number, ticket: Ticket, accountId: string): Promise<void> {
        await this.client.set(`${TICKET_KEY_PREFIX}${worldId}:${ticket}`, accountId, {
            expiration: { type: 'EX', value: this.ttlSeconds },
        });
    }
}

/**
 * 연결하지 않은 Redis 클라이언트를 만든다.
 *
 * 처음 연결에 실패하면 connect()가 그 에러로 끝난다. 기본 재연결 전략은 처음 연결도 계속 다시 시도해서,
 * Redis가 꺼져 있으면 기동이 끝나지도 실패하지도 않는다. 한 번 연결된 뒤에 끊기면 다시 시도한다.
 */
export function createRedisClient({ host, port }: Config['redis']): RedisClient {
    let everConnected = false;

    // node-redis 5는 주소를 socket 아래에서만 읽는다. 최상위에 넘긴 host와 port는 무시되어 기본값으로 붙는다.
    const client = createClient({
        socket: {
            host,
            port,
            reconnectStrategy: (retries, cause) => (everConnected ? Math.min(retries * 100, 3000) : cause),
        },
    });

    client.on('ready', () => {
        everConnected = true;
    });
    // error 리스너가 없으면 연결이 끊길 때 프로세스가 죽는다.
    client.on('error', (err: unknown) => {
        console.error('Redis 오류:', err);
    });

    return client;
}
