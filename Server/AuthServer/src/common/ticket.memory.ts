import type { Ticket, TicketRepository } from './ticket.js';

/** 프로세스 메모리에 인증 티켓을 두는 저장소. 테스트에서 Redis 대신 넣는다. 유효 시간은 다루지 않는다. */
export class MemoryTicketRepository implements TicketRepository {
    private readonly tickets = new Map<string, string>();

    async save(worldId: number, ticket: Ticket, accountId: string): Promise<void> {
        this.tickets.set(`${worldId}:${ticket}`, accountId);
    }

    /** 저장된 티켓을 테스트에서 확인할 때 쓴다. 다른 월드에 발급한 티켓이면 undefined다. */
    accountIdOf(worldId: number, ticket: Ticket): string | undefined {
        return this.tickets.get(`${worldId}:${ticket}`);
    }

    count(): number {
        return this.tickets.size;
    }
}
