import type { Request, RequestHandler } from 'express';

import { success } from '../common/envelope.js';
import { AppError } from '../common/errors.js';
import type { LoginSession } from '../common/loginSession.js';
import { parseWorldId } from '../common/worldDirectory.js';
import type { WorldService } from './world.service.js';

/**
 * Authorization 헤더에서 로그인 세션을 꺼낸다. 헤더가 없거나 Bearer 형식이 아니면 LOGIN_SESSION_EXPIRED를 던진다.
 * 없는 것과 만료된 것을 가르지 않는다. 클라이언트는 어느 쪽이든 다시 로그인한다.
 */
function parseLoginSession(req: Request): LoginSession {
    const match = /^Bearer (\S+)$/.exec(req.get('Authorization') ?? '');
    if (match === null) {
        throw new AppError('LOGIN_SESSION_EXPIRED');
    }
    return match[1];
}

export interface WorldHandler {
    list: RequestHandler;
    select: RequestHandler<{ worldId: string }>;
}

/** 헤더와 경로를 검사해 서비스에 넘기고 성공 봉투로 응답한다. 실패는 던진 에러를 에러 핸들러가 응답한다. */
export function createWorldHandler(service: WorldService): WorldHandler {
    return {
        list: async (req, res) => {
            const result = await service.listWorlds(parseLoginSession(req));
            res.status(200).json(success(result));
        },
        select: async (req, res) => {
            const loginSession = parseLoginSession(req);

            const worldId = parseWorldId(req.params.worldId);
            if (worldId === undefined) {
                throw new AppError('VALIDATION_FAILED', '월드 번호가 올바르지 않습니다.');
            }

            const result = await service.selectWorld(loginSession, worldId);
            res.status(200).json(success(result));
        },
    };
}
