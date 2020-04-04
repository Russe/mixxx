#ifndef EFFECTSLOT_H
#define EFFECTSLOT_H

#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "control/controlencoder.h"
#include "control/controlobject.h"
#include "control/controlpotmeter.h"
#include "control/controlpushbutton.h"
#include "controllers/softtakeover.h"
#include "effects/effectbuttonparameterslot.h"
#include "effects/effectknobparameterslot.h"
#include "effects/effectmanifest.h"
#include "effects/effectparameter.h"
#include "effects/presets/effectpreset.h"
#include "engine/channelhandle.h"
#include "engine/effects/engineeffect.h"
#include "engine/engine.h"
#include "util/class.h"

class EffectProcessor;
class EffectSlot;
class EffectState;
class EffectsManager;
class EngineEffect;
class EngineEffectChain;
class ControlProxy;
class EffectParameter;
class EffectKnobParameterSlot;

typedef QMap<EffectManifestParameter::ParameterType, QList<EffectParameterPointer>> ParameterMap;

class EffectSlot : public QObject {
    Q_OBJECT
  public:
    EffectSlot(const QString& group,
               EffectsManager* pEffectsManager,
               const unsigned int iEffectNumber,
               EngineEffectChain* pEngineEffectChain);
    virtual ~EffectSlot();

    // Call with nullptr for pManifest and pProcessor to unload an effect
    void loadEffect(const EffectManifestPointer pManifest,
            std::unique_ptr<EffectProcessor> pProcessor,
            EffectPresetPointer pPreset,
            const QSet<ChannelHandleAndGroup>& activeChannels,
            bool adoptMetaknobFromPreset = false);

    inline int getEffectSlotNumber() const {
        return m_iEffectNumber;
    }

    inline bool isLoaded() const {
        return m_pEngineEffect != nullptr;
    }

    const QString id() const {
        if (!isLoaded() || m_pManifest == nullptr) {
            return "";
        }
        return m_pManifest->id();
    }

    EffectBackendType backendType() const {
        if (!isLoaded() || m_pManifest == nullptr) {
            return EffectBackendType::BuiltIn;
        }
        return m_pManifest->backendType();
    }

    const ParameterMap getLoadedParameters() const {
        return m_loadedParameters;
    }

    const ParameterMap getHiddenParameters() const {
        ParameterMap hiddenParameters;
        int numTypes = static_cast<int>(EffectManifestParameter::ParameterType::NUM_TYPES);
        for (int parameterTypeId = 0; parameterTypeId < numTypes; ++parameterTypeId) {
            const EffectManifestParameter::ParameterType parameterType =
                    static_cast<EffectManifestParameter::ParameterType>(parameterTypeId);
            for (const auto& pParameter : m_parameters.value(parameterType)) {
                if (!m_loadedParameters.value(parameterType).contains(pParameter)) {
                    hiddenParameters[parameterType].append(pParameter);
                }
            }
        }
        return hiddenParameters;
    }

    void hideParameter(EffectParameterPointer pParameter);
    void showParameter(EffectParameterPointer pParameter);

    void addEffectParameterSlot(EffectManifestParameter::ParameterType parameterType);
    EffectParameterSlotBasePointer getEffectParameterSlot(
            EffectManifestParameter::ParameterType parameterType, unsigned int slotNumber);

    double getMetaParameter() const;

    // Ensures that Softtakover is bypassed for the following
    // ChainParameterChange. Uses for testing only
    void syncSofttakeover();

    const QString& getGroup() const {
        return m_group;
    }

    EffectState* createState(const mixxx::EngineParameters& bufferParameters);

    EffectManifestPointer getManifest() const;

    unsigned int numParameters(EffectManifestParameter::ParameterType parameterType) const;

    void setEnabled(bool enabled);

    void addToEngine(std::unique_ptr<EffectProcessor>,
            const QSet<ChannelHandleAndGroup>& activeInputChannels);
    void removeFromEngine();

  public slots:
    void setMetaParameter(double v, bool force = false);

    void slotNextEffect(double v);
    void slotPrevEffect(double v);
    void slotClear(double v);
    void slotEffectSelector(double v);
    void slotEffectMetaParameter(double v, bool force = false);

  signals:
    void effectChanged();

  private slots:
    void updateEngineState();

  private:
    QString debugString() const {
        return QString("EffectSlot(%1)").arg(m_group);
    }

    void loadParameters();
    void unloadEffect();

    const unsigned int m_iEffectNumber;
    QHash<EffectManifestParameter::ParameterType, unsigned int> m_iNumParameterSlots;
    const QString m_group;
    UserSettingsPointer m_pConfig;
    EffectsManager* m_pEffectsManager;
    EffectManifestPointer m_pManifest;
    EngineEffectChain* m_pEngineEffectChain;
    EngineEffect* m_pEngineEffect;
    ParameterMap m_parameters;
    ParameterMap m_loadedParameters;
    QMap<EffectManifestParameter::ParameterType, QList<EffectParameterSlotBasePointer>> m_parameterSlots;

    ControlObject* m_pControlLoaded;
    QHash<EffectManifestParameter::ParameterType, ControlObject*> m_pControlNumParameters;
    QHash<EffectManifestParameter::ParameterType, ControlObject*> m_pControlNumParameterSlots;
    ControlPushButton* m_pControlEnabled;
    ControlObject* m_pControlNextEffect;
    ControlObject* m_pControlPrevEffect;
    ControlEncoder* m_pControlEffectSelector;
    ControlObject* m_pControlClear;
    ControlPotmeter* m_pControlMetaParameter;

    SoftTakeover* m_pMetaknobSoftTakeover;

    DISALLOW_COPY_AND_ASSIGN(EffectSlot);
};

#endif /* EFFECTSLOT_H */
