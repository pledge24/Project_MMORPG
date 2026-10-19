import type { AccountId, AccountRepository, CreateAccountResult, NewAccount } from './accountRepository.js';

interface StoredAccount extends NewAccount {
    accountId: AccountId;
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
        this.accounts.set(account.loginId, { ...account, accountId });
        return { kind: 'created', accountId };
    }

    /** 저장된 계정을 테스트에서 확인할 때 쓴다. */
    findByLoginId(loginId: string): StoredAccount | undefined {
        return this.accounts.get(loginId);
    }

    count(): number {
        return this.accounts.size;
    }
}
