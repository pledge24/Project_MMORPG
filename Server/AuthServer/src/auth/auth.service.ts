import { randomUUID } from 'node:crypto';

import { AppError } from '../common/errors.js';
import { hashPassword, verifyPassword } from '../common/password.js';
import type { Ticket, TicketRepository } from '../common/ticket.js';
import type { AccountRepository } from './auth.repository.js';

/** 형식 검사를 통과한 로그인 ID와 비밀번호. 형식 검사는 handler가 한다. */
export interface Credentials {
    /** 소문자로 바꾼 값 */
    loginId: string;
    password: string;
}

/**
 * 회원가입과 로그인 판정.
 *
 * 인증 티켓을 발급하는 곳은 로그인뿐이다. 회원가입은 티켓 저장소를 쓰지 않는다.
 */
export class AuthService {
    private readonly accounts: AccountRepository;
    private readonly tickets: TicketRepository;
    private readonly now: () => Date;

    constructor(accounts: AccountRepository, tickets: TicketRepository, now: () => Date = () => new Date()) {
        this.accounts = accounts;
        this.tickets = tickets;
        this.now = now;
    }

    /** 같은 로그인 ID가 있으면 DUPLICATE_LOGIN_ID를 던진다. */
    async register({ loginId, password }: Credentials): Promise<{ loginId: string }> {
        // 중복 확인을 따로 하지 않고 저장소의 유일성 검사에 맡긴다. 확인과 생성 사이에 같은 로그인 ID가 끼어들 수 없다.
        const passwordHash = await hashPassword(password);
        const result = await this.accounts.create({ loginId, passwordHash });
        if (result.kind === 'duplicate') {
            throw new AppError('DUPLICATE_LOGIN_ID');
        }

        return { loginId };
    }

    /**
     * 로그인 ID 조회, 비밀번호 검증, 정지 검사, 마지막 로그인 시각 갱신, 인증 티켓 발급을 이 순서로 한다.
     * 로그인 ID가 없거나 비밀번호가 틀리면 INVALID_CREDENTIALS, 정지 중이면 ACCOUNT_BANNED를 던진다.
     */
    async login({ loginId, password }: Credentials): Promise<{ ticket: Ticket }> {
        // 로그인 ID가 없는 것과 비밀번호가 틀린 것은 같은 에러다. 다르면 로그인 ID가 있는지를 밖에서 알아낼 수 있다.
        const account = await this.accounts.findByLoginId(loginId);
        const passwordMatches = await verifyPassword(password, account?.passwordHash);
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
