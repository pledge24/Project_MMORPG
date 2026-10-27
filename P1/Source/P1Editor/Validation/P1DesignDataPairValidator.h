#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "P1DesignDataPairValidator.generated.h"

/**
 * 기획 데이터 테이블과 에셋 테이블의 짝을 확인하는 에셋 검증기.
 *
 * 프로젝트 설정 「P1 Game Data」가 가리키는 아이템과 몬스터의 수치 테이블이나 에셋 테이블을 검증할 때 두 테이블을 대조한다.
 * 저장, 콘텐츠 브라우저의 검증 메뉴, DataValidation 커맨드렛에서 돈다.
 */
UCLASS()
class UP1DesignDataPairValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

    //~ Begin UEditorValidatorBase Interface
protected:
    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
    virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
    //~ End UEditorValidatorBase Interface
};
