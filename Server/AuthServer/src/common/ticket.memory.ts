import type { Ticket, TicketRepository } from './ticket.js';

/** 프로세스 메모리에 인증 티켓을 두는 저장소. 테스트에서 Redis 대신 넣는다. 유효 시간은 다루지 않는다. */
export class MemoryTicketRepository implements TicketRepository {
    private readonly tickets = new Map<Ticket, string>();

    async save(ticket: Ticket, accountId: string): Promise<void> {
        this.tickets.set(ticket, accountId);
    }

    /** 저장된 티켓을 테스트에서 확인할 때 쓴다. */
    accountIdOf(ticket: Ticket): string | undefined {
        return this.tickets.get(ticket);
    }

    count(): number {
        return this.tickets.size;
    }
}
