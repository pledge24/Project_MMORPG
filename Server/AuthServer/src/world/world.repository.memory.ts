import type { AccountStatus, AccountStatusRepository } from './world.repository.js';

/** 프로세스 메모리에 계정 상태를 두는 저장소. 테스트에서 DB 대신 넣는다. */
export class MemoryAccountStatusRepository implements AccountStatusRepository {
    private readonly statuses = new Map<string, AccountStatus>();

    async findStatus(accountId: string): Promise<AccountStatus | undefined> {
        return this.statuses.get(accountId);
    }

    /** 테스트에서 계정을 두고 정지 기한을 정할 때 쓴다. */
    set(accountId: string, bannedUntil: Date | null): void {
        this.statuses.set(accountId, { bannedUntil });
    }
}
