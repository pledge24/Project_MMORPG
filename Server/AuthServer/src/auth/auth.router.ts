import { Router } from 'express';

import { createAuthHandler } from './auth.handler.js';
import type { AuthService } from './auth.service.js';

/** /api/auth 아래의 경로. 앱에는 이 라우터 하나만 붙인다. */
export function createAuthRouter(service: AuthService): Router {
    const handler = createAuthHandler(service);
    const router = Router();

    router.post('/register', handler.register);
    router.post('/login', handler.login);

    return router;
}
