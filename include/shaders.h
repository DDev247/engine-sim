#ifndef ATG_ENGINE_SIM_SHADERS_H
#define ATG_ENGINE_SIM_SHADERS_H

#include "delta.h"

#include "ui_math.h"

class Shaders : public dbasic::ShaderBase {
    public:
        Shaders();
        ~Shaders();

        ysError Initialize(
                dbasic::ShaderSet *shaderSet,
                ysRenderTarget *mainRenderTarget,
                ysRenderTarget *uiRenderTarget,
                ysShaderProgram *shaderProgram,
                ysInputLayout *inputLayout);
        virtual ysError UseMaterial(dbasic::Material *material);
		virtual void SetObjectTransform(const ysMatrix &mat);
		virtual void ConfigureModel(float scale, dbasic::ModelAsset *model);

        void SetDiffuseTexture(ysTexture *texture);

        void SetBaseColor(const ysVector &color);
        void ResetBaseColor();

        dbasic::StageEnableFlags GetRegularFlags() const;
        dbasic::StageEnableFlags GetUiFlags() const;

        void CalculateCamera(
            float width,
            float height,
            const Bounds &cameraBounds,
            float screenWidth,
            float screenHeight,
            float fovY,
            float phi,
            float theta,
            float zoom = 1.0f,
            float targetX = 0.0f,
            float targetY = 0.0f,
            float targetZ = 0.0f);
        void CalculateUiCamera(float screenWidth, float screenHeight);

        void SetClearColor(const ysVector &col);

    public:
        dbasic::ShaderScreenVariables m_screenVariables;
        dbasic::ShaderScreenVariables m_uiScreenVariables;
        dbasic::ShaderObjectVariables m_objectVariables;

        ysVector m_cameraPosition;
        float m_zoom;

    protected:
        dbasic::ShaderStage *m_mainStage;
        dbasic::ShaderStage *m_uiStage;

        dbasic::LightingControls m_lightingControls;

        dbasic::TextureHandle m_mainStageDiffuseTexture;
};

#endif /* ATG_ENGINE_SIM_SHADERS_H */
