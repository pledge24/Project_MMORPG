import type { AccountId, AccountRepository, CreateAccountResult, LoginAccount, NewAccount } from './accountRepository.js';

interface StoredAccount extends NewAccount {
    accountId: AccountId;
    bannedUntil: Date | null;
    lastLoginAt: Date | null;
}

/** 프로세스 메모리에 계정을 두는 저장소. 테스트에서 DB 대신 넣는다. */
export class MemoryAccountRepository implements AccountRepository {
    private readonly accounts = new Map<string, StoredAccount>();
    private nextAccountId = 1n;

    async create(account: NewAccount): Promise<CreateAccountResult> {
        if (this.accounts.has(account.loginId)) {
            return { kind: 'duplicate' };
        }

        const accountId = String(this.nextAccountId++);
        this.accounts.set(account.loginId, { ...account, accountId, bannedUntil: null, lastLoginAt: null });
        return { kind: 'created', accountId };
    }

    async findByLoginId(loginId: string): Promise<LoginAccount | undefined> {
        const stored = this.accounts.get(loginId);
        if (stored === undefined) {
            return undefined;
        }
        return { accountId: stored.accountId, passwordHash: stored.passwordHash, bannedUntil: stored.bannedUntil };
    }

    async recordLogin(accountId: AccountId): Promise<void> {
        const stored = [...this.accounts.values()].find((account) => account.accountId === accountId);
        if (stored !== undefined) {
            stored.lastLoginAt = new Date();
        }
    }

    /** 저장된 계정을 테스트에서 확인할 때 쓴다. */
    get(loginId: string): Readonly<StoredAccount> | undefined {
        return this.accounts.get(loginId);
    }

    /** 테스트에서 정지 기한을 정할 때 쓴다. */
    setBannedUntil(loginId: string, bannedUntil: Date | null): void {
        const stored = this.accounts.get(loginId);
        if (stored === undefined) {
            throw new Error(`없는 로그인 ID: ${loginId}`);
        }
        stored.bannedUntil = bannedUntil;
    }

    count(): number {
        return this.accounts.size;
    }
}
