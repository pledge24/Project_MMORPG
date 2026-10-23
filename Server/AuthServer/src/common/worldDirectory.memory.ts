import type { WorldDirectory, WorldEntry } from './worldDirectory.js';

/** 프로세스 메모리에 월드 항목을 두는 저장소. 테스트에서 Redis 대신 넣는다. 항목의 만료는 remove로 흉내 낸다. */
export class MemoryWorldDirectory implements WorldDirectory {
    private readonly entries = new Map<number, WorldEntry>();

    async list(): Promise<WorldEntry[]> {
        return [...this.entries.values()].sort((a, b) => a.worldId - b.worldId);
    }

    async find(worldId: number): Promise<WorldEntry | undefined> {
        return this.entries.get(worldId);
    }

    /** 게임 서버가 항목을 갱신한 것처럼 둔다. */
    set(entry: WorldEntry): void {
        this.entries.set(entry.worldId, entry);
    }

    /** 게임 서버가 꺼져 항목이 만료된 것처럼 지운다. */
    remove(worldId: number): void {
        this.entries.delete(worldId);
    }
}
