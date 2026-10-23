import { LOGIN_SESSION_TTL_SECONDS, type LoginSession, type LoginSessionRepository } from './loginSession.js';

interface StoredLoginSession {
    accountId: string;
    expiresAt: Date;
}

/** 프로세스 메모리에 로그인 세션을 두는 저장소. 테스트에서 Redis 대신 넣는다. 수명은 받은 시계로 잰다. */
export class MemoryLoginSessionRepository implements LoginSessionRepository {
    private readonly sessions = new Map<LoginSession, StoredLoginSession>();
    private readonly now: () => Date;

    constructor(now: () => Date = () => new Date()) {
        this.now = now;
    }

    async save(loginSession: LoginSession, accountId: string): Promise<void> {
        const expiresAt = new Date(this.now().getTime() + LOGIN_SESSION_TTL_SECONDS * 1000);
        this.sessions.set(loginSession, { accountId, expiresAt });
    }

    async findAccountId(loginSession: LoginSession): Promise<string | undefined> {
        const stored = this.sessions.get(loginSession);
        if (stored === undefined || stored.expiresAt <= this.now()) {
            return undefined;
        }
        return stored.accountId;
    }

    count(): number {
        return this.sessions.size;
    }
}
