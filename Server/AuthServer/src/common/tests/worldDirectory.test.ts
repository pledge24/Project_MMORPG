import { describe, expect, test } from 'vitest';

import { parseWorldEntry, parseWorldId } from '../worldDirectory.js';

// Redis에 붙지 않는다. 게임 서버가 월드 항목의 해시에 쓰는 필드를 읽는 규칙만 본다. 필드 이름과 형식은 게임 서버와의 계약이다.

const VALID_FIELDS = { name: '하나', host: '10.0.0.1', port: '7777', congestion: 'SMOOTH' };

describe('parseWorldEntry', () => {
    test('게임 서버가 쓴 필드를 월드 항목으로 읽는다', () => {
        expect(parseWorldEntry(3, VALID_FIELDS)).toEqual({ worldId: 3, name: '하나', host: '10.0.0.1', port: 7777, congestion: 'SMOOTH' });
    });

    test.each(['SMOOTH', 'NORMAL', 'CROWDED'])('혼잡도 %s를 읽는다', (congestion) => {
        expect(parseWorldEntry(1, { ...VALID_FIELDS, congestion })?.congestion).toBe(congestion);
    });

    test.each([
        ['이름 없음', { ...VALID_FIELDS, name: undefined }],
        ['빈 이름', { ...VALID_FIELDS, name: '' }],
        ['주소 없음', { ...VALID_FIELDS, host: undefined }],
        ['포트가 숫자가 아님', { ...VALID_FIELDS, port: 'abc' }],
        ['포트가 0', { ...VALID_FIELDS, port: '0' }],
        ['포트가 범위 밖', { ...VALID_FIELDS, port: '65536' }],
        ['모르는 혼잡도', { ...VALID_FIELDS, congestion: 'FULL' }],
        ['혼잡도 없음', { ...VALID_FIELDS, congestion: undefined }],
    ])('%s이면 읽지 않는다', (_case, fields) => {
        expect(parseWorldEntry(1, fields as Record<string, string>)).toBeUndefined();
    });
});

describe('parseWorldId', () => {
    test.each([['1', 1], ['2147483647', 2147483647]])('%s를 읽는다', (text, worldId) => {
        expect(parseWorldId(text)).toBe(worldId);
    });

    test.each(['', '0', '01', '-1', '+1', '1.5', ' 1', '2147483648'])('%s는 월드 번호가 아니다', (text) => {
        expect(parseWorldId(text)).toBeUndefined();
    });
});
