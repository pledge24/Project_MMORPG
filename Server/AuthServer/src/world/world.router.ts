import { Router } from 'express';

import { createWorldHandler } from './world.handler.js';
import type { WorldService } from './world.service.js';

/** /api/worlds 아래의 경로. 앱에는 이 라우터 하나만 붙인다. */
export function createWorldRouter(service: WorldService): Router {
    const handler = createWorldHandler(service);
    const router = Router();

    router.get('/', handler.list);
    router.post('/:worldId/select', handler.select);

    return router;
}
