import { createClient } from 'redis';

import type { AccountId } from '../accounts/accountRepository.js';
import type { Config } from '../config/config.js';
import type { Ticket, TicketRepository } from './ticketRepository.js';

export type RedisClient = ReturnType<typeof createClient>;

// 키 이름은 게임 서버와의 계약이다. 바꾸면 게임 서버가 티켓을 찾지 못한다.
const TICKET_KEY_PREFIX = 'authTicket:';

/** 인증 티켓을 Redis의 키 하나로 두는 저장소. 값은 계정 번호 문자열 하나이고 TTL이 붙는다. */
export class RedisTicketRepository implements TicketRepository {
    private readonly client: RedisClient;
    private readonly ttlSeconds: number;

    constructor(client: RedisClient, ttlSeconds: number) {
        this.client = client;
        this.ttlSeconds = ttlSeconds;
    }

    async save(ticket: Ticket, accountId: AccountId): Promise<void> {
        await this.client.set(`${TICKET_KEY_PREFIX}${ticket}`, accountId, {
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
