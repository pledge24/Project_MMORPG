import { Router } from 'express';

import type { RegisterService } from '../auth/registerService.js';
import { success } from './envelope.js';

/** /api/auth 아래의 경로. 판정은 서비스에 넘기고, 실패는 서비스가 던진 에러를 에러 핸들러가 응답한다. */
export function createAuthRouter(registerService: RegisterService): Router {
    const router = Router();

    router.post('/register', async (req, res) => {
        const result = await registerService.register(req.body);
        res.status(201).json(success(result));
    });

    return router;
}
