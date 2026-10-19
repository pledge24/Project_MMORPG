import bcrypt from 'bcrypt';
import { randomUUID } from 'node:crypto';

import type { AccountRepository } from '../accounts/accountRepository.js';
import { AppError } from '../errors.js';
import type { Ticket, TicketRepository } from '../tickets/ticketRepository.js';
import { BCRYPT_COST, parseCredentials } from './credentials.js';

// 없는 로그인 ID에도 비밀번호 검증을 한 번 돌린다. 건너뛰면 응답 시간의 차이로 로그인 ID가 있는지를 밖에서 알아낼 수 있다.
const PASSWORD_HASH_FOR_UNKNOWN_ID = bcrypt.hashSync(randomUUID(), BCRYPT_COST);

/**
 * 로그인 판정.
 *
 * 로그인 ID 조회, 비밀번호 검증, 정지 검사, 마지막 로그인 시각 갱신, 인증 티켓 발급을 이 순서로 한다.
 * 인증 티켓을 발급하는 곳은 여기뿐이다.
 */
export class LoginService {
    private readonly accounts: AccountRepository;
    private readonly tickets: TicketRepository;
    private readonly now: () => Date;

    constructor(accounts: AccountRepository, tickets: TicketRepository, now: () => Date = () => new Date()) {
        this.accounts = accounts;
        this.tickets = tickets;
        this.now = now;
    }

    /**
     * 형식이 틀리면 VALIDATION_FAILED, 로그인 ID가 없거나 비밀번호가 틀리면 INVALID_CREDENTIALS,
     * 정지 중이면 ACCOUNT_BANNED를 던진다.
     */
    async login(body: unknown): Promise<{ ticket: Ticket }> {
        const { loginId, password } = parseCredentials(body);

        // 로그인 ID가 없는 것과 비밀번호가 틀린 것은 같은 에러다. 다르면 로그인 ID가 있는지를 밖에서 알아낼 수 있다.
        const account = await this.accounts.findByLoginId(loginId);
        const passwordMatches = await bcrypt.compare(password, account?.passwordHash ?? PASSWORD_HASH_FOR_UNKNOWN_ID);
        if (account === undefined || !passwordMatches) {
            throw new AppError('INVALID_CREDENTIALS');
        }

        // 정지 검사는 비밀번호 검증 뒤에 한다. 앞에 두면 비밀번호를 모르는 사람도 그 로그인 ID가 정지되었다는 것을 안다.
        if (account.bannedUntil !== null && account.bannedUntil > this.now()) {
            throw new AppError('ACCOUNT_BANNED', undefined, { bannedUntil: account.bannedUntil.toISOString() });
        }

        await this.accounts.recordLogin(account.accountId);

        const ticket = randomUUID();
        await this.tickets.save(ticket, account.accountId);
        return { ticket };
    }
}
