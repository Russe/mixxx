#ifndef EFFECTSMANAGER_H
#define EFFECTSMANAGER_H

#include <QObject>
#include <QHash>
#include <QList>
#include <QSet>
#include <QScopedPointer>
#include <QPair>

#include "control/controlpotmeter.h"
#include "control/controlpushbutton.h"
#include "effects/specialeffectchainslots.h"
#include "effects/effectmanifestparameter.h"
#include "engine/channelhandle.h"
#include "engine/effects/message.h"
#include "preferences/usersettings.h"
#include "util/class.h"
#include "util/fifo.h"
#include "util/memory.h"
#include "util/xml.h"

class EngineEffectsManager;
class EffectManifest;

typedef QMap<EffectManifestParameter::ParameterType, QList<EffectParameterPointer>> ParameterMap;

class EffectsManager : public QObject {
    Q_OBJECT
  public:
    typedef bool (*EffectManifestFilterFnc)(EffectManifest* pManifest);

    EffectsManager(QObject* pParent, UserSettingsPointer pConfig,
                   ChannelHandleFactory* pChannelHandleFactory);
    virtual ~EffectsManager();

    EngineEffectsManager* getEngineEffectsManager() {
        return m_pEngineEffectsManager;
    }

    const ChannelHandle getMasterHandle() {
        return m_pChannelHandleFactory->getOrCreateHandle("[Master]");
    }

    // Add an effect backend to be managed by EffectsManager. EffectsManager
    // takes ownership of the backend, and will delete it when EffectsManager is
    // being deleted. Not thread safe -- use only from the GUI thread.
    void addEffectsBackend(EffectsBackendPointer pEffectsBackend);

    // To support cycling through effect chains, there is a global ordering of
    // chains. These methods allow you to get the next or previous chain given
    // your current chain.
    // EffectChainSlotPointer getNextEffectChain(EffectChainSlotPointer pEffectChainSlot);
    // EffectChainSlotPointer getPrevEffectChain(EffectChainSlotPointer pEffectChainSlot);

    // NOTE(Kshitij) : New functions for saving and loading
    // bool saveEffectChains();

    static const int kNumStandardEffectChains = 4;

    bool isAdoptMetaknobValueEnabled() const;

    void registerInputChannel(const ChannelHandleAndGroup& handle_group);
    const QSet<ChannelHandleAndGroup>& registeredInputChannels() const {
        return m_registeredInputChannels;
    }

    void registerOutputChannel(const ChannelHandleAndGroup& handle_group);
    const QSet<ChannelHandleAndGroup>& registeredOutputChannels() const {
        return m_registeredOutputChannels;
    }

    void loadStandardEffect(
            const int iChainSlotNumber,
            const int iEffectSlotNumber,
            const EffectManifestPointer pManifest);

    void loadOutputEffect(
            const int iEffectSlotNumber,
            const EffectManifestPointer pManifest);

    void loadQuickEffect(
            const QString& group,
            const int iEffectSlotNumber,
            const EffectManifestPointer pManifest);

    void loadEqualizerEffect(const QString& group,
            const int iEffectSlotNumber,
            const EffectManifestPointer pManifest);

    void loadEffect(
            EffectChainSlotPointer pChainSlot,
            const int iEffectSlotNumber,
            const EffectManifestPointer pManifest,
            EffectPresetPointer pPreset = nullptr,
            bool adoptMetaknobFromPreset = false);

    std::unique_ptr<EffectProcessor> createProcessor(
            const EffectManifestPointer pManifest);

    ParameterMap getLoadedParameters(int chainNumber, int effectNumber) const;
    ParameterMap getHiddenParameters(int chainNumber, int effectNumber) const;

    void hideParameter(int chainNumber, int effectNumber, EffectParameterPointer pParameter);
    void showParameter(int chainNumber, int effectNumber, EffectParameterPointer pParameter);

    void loadEffectChainPreset(EffectChainSlotPointer pChainSlot,
            EffectChainPresetPointer pPreset);
    void loadPresetToStandardChain(int chainNumber, EffectChainPresetPointer pPreset);
    void addStandardEffectChainSlots();
    EffectChainSlotPointer getStandardEffectChainSlot(int unitNumber) const;

    void addOutputEffectChainSlot();
    EffectChainSlotPointer getOutputEffectChainSlot() const;

    void addEqualizerEffectChainSlot(const QString& deckGroupName);
    void addQuickEffectChainSlot(const QString& deckGroupName);

    // TODO: Remove these methods to reduce coupling between GUI and
    // effects system implementation details.
    EffectChainSlotPointer getEffectChainSlot(const QString& group) const;
    EffectSlotPointer getEffectSlot(const QString& group);

    EffectParameterSlotBasePointer getEffectParameterSlot(
            const EffectManifestParameter::ParameterType parameterType, const ConfigKey& configKey);

    QString getNextEffectId(const QString& effectId);
    QString getPrevEffectId(const QString& effectId);

    inline const QList<EffectManifestPointer>& getAvailableEffectManifests() const {
        return m_availableEffectManifests;
    };
    inline const QList<EffectManifestPointer>& getVisibleEffectManifests() const {
        return m_visibleEffectManifests;
    };
    const QList<EffectManifestPointer> getAvailableEffectManifestsFiltered(
        EffectManifestFilterFnc filter) const;
    bool isEQ(const QString& effectId) const;

    void getEffectManifestAndBackend(
            const QString& effectId,
            EffectManifestPointer* ppManifest, EffectsBackend** ppBackend) const;
    EffectManifestPointer getManifestFromUniqueId(const QString& uid) const;
    EffectManifestPointer getManifest(const QString& id, EffectBackendType backendType) const;

    void setEffectVisibility(EffectManifestPointer pManifest, bool visibility);
    bool getEffectVisibility(EffectManifestPointer pManifest);

    void saveDefaultForEffect(EffectPresetPointer pEffectPreset);
    void saveDefaultForEffect(int chainNumber, int effcectNumber);

    void savePresetFromStandardEffectChain(int chainNumber);
    const QList<EffectChainPresetPointer> getAvailableChainPresets() const;

    void setup();

    // Write an EffectsRequest to the EngineEffectsManager. EffectsManager takes
    // ownership of request and deletes it once a response is received.
    bool writeRequest(EffectsRequest* request);

  signals:
    void visibleEffectsUpdated();

  private:
    QString debugString() const {
        return "EffectsManager";
    }

    void loadDefaultEffectPresets();
    void loadEffectChainPresets();

    void reloadStandardEffectChains();
    void saveStandardEffectChains();

    void processEffectsResponses();
    void collectGarbage(const EffectsRequest* pResponse);

    ChannelHandleFactory* m_pChannelHandleFactory;

    QHash<EffectBackendType, EffectsBackendPointer> m_effectsBackends;
    QList<EffectManifestPointer> m_availableEffectManifests;
    QList<EffectManifestPointer> m_visibleEffectManifests;

    EngineEffectsManager* m_pEngineEffectsManager;

    QScopedPointer<EffectsRequestPipe> m_pRequestPipe;
    qint64 m_nextRequestId;
    QHash<qint64, EffectsRequest*> m_activeRequests;

    ControlObject* m_pNumEffectsAvailable;
    // We need to create Control Objects for Equalizers' frequencies
    ControlPotmeter m_loEqFreq;
    ControlPotmeter m_hiEqFreq;

    bool m_underDestruction;

    QSet<ChannelHandleAndGroup> m_registeredInputChannels;
    QSet<ChannelHandleAndGroup> m_registeredOutputChannels;
    UserSettingsPointer m_pConfig;
    QHash<QString, EffectChainSlotPointer> m_effectChainSlotsByGroup;

    QList<StandardEffectChainSlotPointer> m_standardEffectChainSlots;
    OutputEffectChainSlotPointer m_outputEffectChainSlot;
    QHash<QString, EqualizerEffectChainSlotPointer> m_equalizerEffectChainSlots;
    QHash<QString, QuickEffectChainSlotPointer> m_quickEffectChainSlots;

    QHash<QString, EffectChainPresetPointer> m_effectChainPresets;
    QHash<EffectManifestPointer, EffectPresetPointer> m_defaultPresets;

    DISALLOW_COPY_AND_ASSIGN(EffectsManager);
};


#endif /* EFFECTSMANAGER_H */
