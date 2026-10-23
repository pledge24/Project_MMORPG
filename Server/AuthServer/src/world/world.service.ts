import { randomUUID } from 'node:crypto';

import { AppError } from '../common/errors.js';
import type { LoginSession, LoginSessionRepository } from '../common/loginSession.js';
import type { Ticket, TicketRepository } from '../common/ticket.js';
import { toWorldSummary, type WorldDirectory, type WorldSummary } from '../common/worldDirectory.js';
import type { AccountStatusRepository } from './world.repository.js';

export interface WorldSelection {
    host: string;
    port: number;
    ticket: Ticket;
}

/**
 * 월드 목록과 월드 선택 판정.
 *
 * 인증 티켓을 발급하는 곳은 월드 선택뿐이다. 월드 목록은 게임 서버가 알린 항목을 그대로 전달한다.
 */
export class WorldService {
    private readonly loginSessions: LoginSessionRepository;
    private readonly worlds: WorldDirectory;
    private readonly accounts: AccountStatusRepository;
    private readonly tickets: TicketRepository;
    private readonly now: () => Date;

    constructor(
        loginSessions: LoginSessionRepository,
        worlds: WorldDirectory,
        accounts: AccountStatusRepository,
        tickets: TicketRepository,
        now: () => Date = () => new Date(),
    ) {
        this.loginSessions = loginSessions;
        this.worlds = worlds;
        this.accounts = accounts;
        this.tickets = tickets;
        this.now = now;
    }

    /** 로그인 세션이 없거나 만료됐으면 LOGIN_SESSION_EXPIRED를 던진다. */
    async listWorlds(loginSession: LoginSession): Promise<{ worlds: WorldSummary[] }> {
        await this.requireAccountId(loginSession);

        const entries = await this.worlds.list();
        return { worlds: entries.map(toWorldSummary) };
    }

    /**
     * 로그인 세션 확인, 정지 검사, 월드 항목 조회, 인증 티켓 발급을 이 순서로 한다.
     * 로그인 세션이 없거나 만료됐으면 LOGIN_SESSION_EXPIRED, 정지 중이면 ACCOUNT_BANNED, 월드 항목이 없으면 WORLD_UNAVAILABLE을 던진다.
     */
    async selectWorld(loginSession: LoginSession, worldId: number): Promise<WorldSelection> {
        const accountId = await this.requireAccountId(loginSession);

        // 로그인 세션은 1시간 산다. 그사이 정지된 계정이 월드에 들어가지 못하도록 티켓을 발급하기 전에 다시 본다.
        const status = await this.accounts.findStatus(accountId);
        if (status === undefined) {
            throw new AppError('LOGIN_SESSION_EXPIRED');
        }
        if (status.bannedUntil !== null && status.bannedUntil > this.now()) {
            throw new AppError('ACCOUNT_BANNED', undefined, { bannedUntil: status.bannedUntil.toISOString() });
        }

        // 혼잡도는 보지 않는다. 혼잡한 월드도 고를 수 있다.
        const world = await this.worlds.find(worldId);
        if (world === undefined) {
            throw new AppError('WORLD_UNAVAILABLE');
        }

        const ticket = randomUUID();
        await this.tickets.save(worldId, ticket, accountId);
        return { host: world.host, port: world.port, ticket };
    }

    private async requireAccountId(loginSession: LoginSession): Promise<string> {
        const accountId = await this.loginSessions.findAccountId(loginSession);
        if (accountId === undefined) {
            throw new AppError('LOGIN_SESSION_EXPIRED');
        }
        return accountId;
    }
}
