#pragma once

/**
 * 로비와 게임의 기획 데이터 대조.
 *
 * 로비와 게임은 서로의 기획 데이터를 읽지 않으므로, 둘이 맞아야 하는 값은 기동할 때 배선이 대조한다.
 */
namespace DataCrossCheck
{
    /** 설치된 로비와 게임의 기획 데이터를 대조한다. 맞으면 nullopt, 어긋나면 사유. */
    optional<string> Check();
}
