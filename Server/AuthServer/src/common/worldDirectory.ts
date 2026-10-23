import type { RedisClient } from './ticket.js';

/** 혼잡도. 게임 서버가 자기 정원에 비춰 정한 단계를 그대로 전달한다. 월드 선택을 막지 않는다. */
export const CONGESTIONS = ['SMOOTH', 'NORMAL', 'CROWDED'] as const;

export type Congestion = (typeof CONGESTIONS)[number];

/** 월드 목록의 항목. 클라이언트에 보내는 모양이다. */
export interface WorldSummary {
    worldId: number;
    name: string;
    congestion: Congestion;
}

/** 게임 서버가 알린 월드 항목. 접속 주소는 월드를 고른 계정에만 알린다. */
export interface WorldEntry extends WorldSummary {
    host: string;
    port: number;
}

/**
 * 게임 서버가 알린 월드 항목을 읽는 곳.
 *
 * 판정 코드는 이 인터페이스만 안다. Redis를 아는 코드는 구현뿐이다. 쓰는 쪽은 게임 서버이므로 여기에 쓰기가 없다.
 */
export interface WorldDirectory {
    /** 항목이 살아 있는 월드를 월드 번호 순으로 돌려준다. */
    list(): Promise<WorldEntry[]>;

    /** 항목이 살아 있지 않으면 undefined다. */
    find(worldId: number): Promise<WorldEntry | undefined>;
}

export function toWorldSummary({ worldId, name, congestion }: WorldEntry): WorldSummary {
    return { worldId, name, congestion };
}

/** 월드 번호의 형식. 앞자리 0이 없는 1 이상의 32비트 정수다. 게임 서버 설정의 범위와 같다. */
export function parseWorldId(text: string): number | undefined {
    if (!/^[1-9]\d{0,9}$/.test(text)) {
        return undefined;
    }
    const worldId = Number(text);
    return worldId <= 2147483647 ? worldId : undefined;
}

/**
 * 게임 서버가 쓴 월드 항목의 필드를 읽는다. 빠지거나 형식이 틀린 필드가 있으면 undefined다.
 * 필드 이름과 값의 형식은 게임 서버와의 계약이다.
 */
export function parseWorldEntry(worldId: number, fields: Record<string, string>): WorldEntry | undefined {
    const { name, host, port, congestion } = fields;
    if (name === undefined || name === '' || host === undefined || host === '') {
        return undefined;
    }

    const parsedPort = port !== undefined && /^[1-9]\d{0,4}$/.test(port) ? Number(port) : NaN;
    if (!(parsedPort <= 65535)) {
        return undefined;
    }

    if (!CONGESTIONS.includes(congestion as Congestion)) {
        return undefined;
    }

    return { worldId, name, host, port: parsedPort, congestion: congestion as Congestion };
}

// 키 이름은 게임 서버와의 계약이다. 바꾸면 게임 서버가 알린 월드를 찾지 못한다.
const WORLD_SET_KEY = 'worlds';
const WORLD_KEY_PREFIX = 'world:';

/**
 * 월드 번호 집합과 월드마다의 해시로 월드 항목을 읽는 저장소.
 *
 * 게임 서버는 기동할 때 집합에 자기 번호를 넣고, 자기 항목을 TTL을 붙여 주기적으로 갱신한다.
 * 꺼진 월드는 번호가 집합에 남아도 항목이 만료되어 목록에서 빠진다.
 */
export class RedisWorldDirectory implements WorldDirectory {
    private readonly client: RedisClient;

    constructor(client: RedisClient) {
        this.client = client;
    }

    async list(): Promise<WorldEntry[]> {
        const worldIds = (await this.client.sMembers(WORLD_SET_KEY))
            .map((member) => parseWorldId(member))
            .filter((worldId) => worldId !== undefined)
            .sort((a, b) => a - b);

        const entries = await Promise.all(worldIds.map((worldId) => this.find(worldId)));
        return entries.filter((entry) => entry !== undefined);
    }

    async find(worldId: number): Promise<WorldEntry | undefined> {
        const fields = await this.client.hGetAll(`${WORLD_KEY_PREFIX}${worldId}`);

        // 키가 없으면 빈 객체가 온다. 만료된 월드라 목록에서 빠지는 것이 정상이다.
        if (Object.keys(fields).length === 0) {
            return undefined;
        }

        const entry = parseWorldEntry(worldId, fields);
        if (entry === undefined) {
            // 게임 서버와 계약이 어긋났다. 응답에는 싣지 않으므로 여기서 남긴다. 없으면 월드가 왜 안 보이는지 모른다.
            console.error(`월드 ${worldId}의 항목 형식이 틀려 목록에서 뺀다:`, fields);
        }
        return entry;
    }
}
