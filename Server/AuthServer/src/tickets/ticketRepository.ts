import type { AccountId } from '../accounts/accountRepository.js';

/** 인증 티켓. crypto.randomUUID()가 만든 UUID 문자열이다. */
export type Ticket = string;

/**
 * 인증 티켓을 두는 곳.
 *
 * 로그인 판정은 이 인터페이스만 안다. Redis를 아는 코드는 구현뿐이다. 소비는 게임 서버가 하므로 여기에 없다.
 */
export interface TicketRepository {
    /** 티켓이 가리키는 계정 번호를 저장한다. 유효 시간은 구현이 정한다. */
    save(ticket: Ticket, accountId: AccountId): Promise<void>;
}
