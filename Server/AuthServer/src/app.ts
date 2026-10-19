import express, { type Express } from 'express';

import type { LoginService } from './auth/loginService.js';
import type { RegisterService } from './auth/registerService.js';
import { createAuthRouter } from './http/authRouter.js';
import { errorHandler, notFoundHandler } from './http/errorHandlers.js';

export interface AppDependencies {
    registerService: RegisterService;
    loginService: LoginService;
}

/** 받은 의존성으로 앱을 조립한다. 포트는 열지 않는다. */
export function createApp({ registerService, loginService }: AppDependencies): Express {
    const app = express();

    app.use(express.json());
    app.use('/api/auth', createAuthRouter(registerService, loginService));

    app.use(notFoundHandler);
    app.use(errorHandler);

    return app;
}
