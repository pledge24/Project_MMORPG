import type { RedisClient } from './ticket.js';

/** 로그인 세션. crypto.randomUUID()가 만든 UUID 문자열이다. */
export type LoginSession = string;

/** 로그인 세션의 수명(초). 로그인한 시각부터 고정이고, 월드 목록이나 월드 선택에 써도 늘어나지 않는다. */
export const LOGIN_SESSION_TTL_SECONDS = 60 * 60;

/**
 * 로그인 세션을 두는 곳.
 *
 * 판정 코드는 이 인터페이스만 안다. Redis를 아는 코드는 구현뿐이다.
 * 계정 번호는 10진수 문자열로 받는다. 공용 코드가 도메인의 타입을 import하지 않도록 문자열로 둔다.
 */
export interface LoginSessionRepository {
    /** 로그인 세션이 가리키는 계정 번호를 저장한다. 수명은 저장한 시각부터 LOGIN_SESSION_TTL_SECONDS다. */
    save(loginSession: LoginSession, accountId: string): Promise<void>;

    /** 살아 있는 로그인 세션이 가리키는 계정 번호. 없거나 만료됐으면 undefined다. 수명을 늘리지 않는다. */
    findAccountId(loginSession: LoginSession): Promise<string | undefined>;
}

// 키 이름은 인증 서버 안에서만 쓴다. 게임 서버는 로그인 세션을 모른다.
const LOGIN_SESSION_KEY_PREFIX = 'loginSession:';

/** 로그인 세션을 Redis의 키 하나로 두는 저장소. 값은 계정 번호 문자열 하나이고 TTL이 붙는다. */
export class RedisLoginSessionRepository implements LoginSessionRepository {
    private readonly client: RedisClient;

    constructor(client: RedisClient) {
        this.client = client;
    }

    async save(loginSession: LoginSession, accountId: string): Promise<void> {
        await this.client.set(`${LOGIN_SESSION_KEY_PREFIX}${loginSession}`, accountId, {
            expiration: { type: 'EX', value: LOGIN_SESSION_TTL_SECONDS },
        });
    }

    async findAccountId(loginSession: LoginSession): Promise<string | undefined> {
        // GET은 TTL을 건드리지 않는다. 쓸 때마다 수명이 늘어나면 탈취한 로그인 세션이 끝없이 살아 있다.
        const accountId = await this.client.get(`${LOGIN_SESSION_KEY_PREFIX}${loginSession}`);
        return accountId ?? undefined;
    }
}
