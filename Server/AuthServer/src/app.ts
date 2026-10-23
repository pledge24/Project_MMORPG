import express, { type Express } from 'express';

import type { AuthService } from './auth/auth.service.js';
import { createAuthRouter } from './auth/auth.router.js';
import { errorHandler, notFoundHandler } from './common/errorHandlers.js';
import { createWorldRouter } from './world/world.router.js';
import type { WorldService } from './world/world.service.js';

export interface AppDependencies {
    authService: AuthService;
    worldService: WorldService;
}

/** 받은 의존성으로 앱을 조립한다. 포트는 열지 않는다. 도메인마다 라우터를 한 줄로 붙인다. */
export function createApp({ authService, worldService }: AppDependencies): Express {
    const app = express();

    app.use(express.json());
    app.use('/api/auth', createAuthRouter(authService));
    app.use('/api/worlds', createWorldRouter(worldService));

    app.use(notFoundHandler);
    app.use(errorHandler);

    return app;
}
