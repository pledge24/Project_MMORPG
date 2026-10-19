import express, { type Express, type Router } from 'express';

import type { RegisterService } from './auth/registerService.js';
import { createAuthRouter } from './http/authRouter.js';
import { errorHandler, notFoundHandler } from './http/errorHandlers.js';

export interface AppDependencies {
    registerService: RegisterService;
    /** 아직 새 구조로 옮기지 않은 옛 경로. 옮기고 나면 지운다. */
    legacyRouter?: Router;
}

/** 받은 의존성으로 앱을 조립한다. 포트는 열지 않는다. */
export function createApp({ registerService, legacyRouter }: AppDependencies): Express {
    const app = express();

    app.use(express.json());
    app.use('/api/auth', createAuthRouter(registerService));
    if (legacyRouter !== undefined) {
        app.use(legacyRouter);
    }

    app.use(notFoundHandler);
    app.use(errorHandler);

    return app;
}
