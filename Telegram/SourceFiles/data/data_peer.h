/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/notify/data_peer_notify_settings.h"
#include "data/data_types.h"
#include "data/data_flags.h"
#include "data/data_cloud_file.h"
#include "data/data_peer_common.h"
#include "ui/userpic_view.h"

struct BotInfo;
class PeerData;
class UserData;
class ChatData;
class ChannelData;

enum class ChatRestriction;

namespace Ui {
class EmptyUserpic;
struct BotVerifyDetails;
struct ColorCollectible;
} // namespace Ui

namespace Main {
class Account;
class Session;
} // namespace Main

namespace Data {

class Forum;
class ForumTopic;
class Session;
class GroupCall;
class SavedMessages;
class SavedSublist;
struct ReactionId;
class WallPaper;

[[nodiscard]] uint8 DecideColorIndex(PeerId peerId);

// Must be used only for PeerColor-s.
[[nodiscard]] PeerId FakePeerIdForJustName(const QString &name);

class RestrictionCheckResult {
public:
	[[nodiscard]] static RestrictionCheckResult Allowed() {
		return { 0 };
	}
	[[nodiscard]] static RestrictionCheckResult WithEveryone() {
		return { 1 };
	}
	[[nodiscard]] static RestrictionCheckResult Explicit() {
		return { 2 };
	}

	explicit operator bool() const {
		return (_value != 0);
	}

	bool operator==(const RestrictionCheckResult &other) const {
		return (_value == other._value);
	}
	bool operator!=(const RestrictionCheckResult &other) const {
		return !(*this == other);
	}

	[[nodiscard]] bool isAllowed() const {
		return (*this == Allowed());
	}
	[[nodiscard]] bool isWithEveryone() const {
		return (*this == WithEveryone());
	}
	[[nodiscard]] bool isExplicit() const {
		return (*this == Explicit());
	}

private:
	RestrictionCheckResult(int value) : _value(value) {
	}

	int _value = 0;

};

struct UnavailableReason {
	QString reason;
	QString text;

	friend inline bool operator==(
		const UnavailableReason &,
		const UnavailableReason &) = default;

	[[nodiscard]] bool sensitive() const;
	[[nodiscard]] static UnavailableReason Sensitive();

	[[nodiscard]] static QString Compute(
		not_null<Main::Session*> session,
		const std::vector<UnavailableReason> &list);
	[[nodiscard]] static bool IgnoreSensitiveMark(
		not_null<Main::Session*> session);

	[[nodiscard]] static std::vector<UnavailableReason> Extract(
		const MTPvector<MTPRestrictionReason> *list);
};

bool ApplyBotMenuButton(
	not_null<BotInfo*> info,
	const MTPBotMenuButton *button);

enum class AllowedReactionsType : uchar {
	All,
	Default,
	Some,
};

struct AllowedReactions {
	std::vector<ReactionId> some;
	int maxCount = 0;
	AllowedReactionsType type = AllowedReactionsType::Some;
	bool paidEnabled = false;

	friend inline bool operator==(
		const AllowedReactions &,
		const AllowedReactions &) = default;
};

[[nodiscard]] AllowedReactions Parse(
	const MTPChatReactions &value,
	int maxCount,
	bool paidEnabled);
[[nodiscard]] PeerData *PeerFromInputMTP(
	not_null<Session*> owner,
	const MTPInputPeer &input);
[[nodiscard]] UserData *UserFromInputMTP(
	not_null<Session*> owner,
	const MTPInputUser &input);

[[nodiscard]] Ui::ColorCollectible ParseColorCollectible(
	const MTPDpeerColorCollectible &data);

} // namespace Data

class PeerClickHandler : public ClickHandler {
public:
	PeerClickHandler(not_null<PeerData*> peer);
	void onClick(ClickContext context) const override;

	not_null<PeerData*> peer() const {
		return _peer;
	}

private:
	not_null<PeerData*> _peer;

};

enum class PeerBarSetting {
	ReportSpam = (1 << 0),
	AddContact = (1 << 1),
	BlockContact = (1 << 2),
	ShareContact = (1 << 3),
	NeedContactsException = (1 << 4),
	AutoArchived = (1 << 5),
	RequestChat = (1 << 6),
	RequestChatIsBroadcast = (1 << 7),
	HasBusinessBot = (1 << 8),
	BusinessBotPaused = (1 << 9),
	BusinessBotCanReply = (1 << 10),
	Unknown = (1 << 11),
};
inline constexpr bool is_flag_type(PeerBarSetting) { return true; };
using PeerBarSettings = base::flags<PeerBarSetting>;

struct PeerBarDetails {
	QString phoneCountryCode;
	int registrationDate = 0; // YYYYMM or 0, YYYY > 2012, MM > 0.
	TimeId nameChangeDate = 0;
	TimeId photoChangeDate = 0;
	QString requestChatTitle;
	TimeId requestChatDate;
	UserData *businessBot = nullptr;
	QString businessBotManageUrl;
	int paysPerMessage = 0;
};

struct PaintUserpicContext {
	QPoint position;
	int size = 0;
	Ui::PeerUserpicShape shape = Ui::PeerUserpicShape::Auto;
};

class PeerData {
protected:
	PeerData(not_null<Data::Session*> owner, PeerId id);
	PeerData(const PeerData &other) = delete;
	PeerData &operator=(const PeerData &other) = delete;

public:
	using BarSettings = Data::Flags<PeerBarSettings>;

	virtual ~PeerData();

	static constexpr auto kServiceNotificationsId = peerFromUser(777000);
	static constexpr auto kSavedHiddenAuthorId = peerFromUser(2666000);

	[[nodiscard]] Data::Session &owner() const;
	[[nodiscard]] Main::Session &session() const;
	[[nodiscard]] Main::Account &account() const;

	[[nodiscard]] uint8 colorIndex() const {
		return _colorIndex;
	}
	[[nodiscard]] auto colorCollectible() const
	-> const std::shared_ptr<Ui::ColorCollectible> & {
		return _colorCollectible;
	}
	bool changeColorCollectible(Ui::ColorCollectible data);
	bool clearColorCollectible();
	bool changeColorIndex(uint8 index);
	bool clearColorIndex();
	[[nodiscard]] DocumentId backgroundEmojiId() const;
	bool changeBackgroundEmojiId(DocumentId id);

	[[nodiscard]] std::optional<uint8> colorProfileIndex() const {
		return _colorProfileIndex;
	}
	[[nodiscard]] auto colorProfileCollectible() const
	-> const std::shared_ptr<Ui::ColorCollectible> & {
		return _colorProfileCollectible;
	}
	bool changeColorProfileCollectible(Ui::ColorCollectible data);
	bool changeColorProfileCollectible(
		const tl::conditional<MTPPeerColor> &cloudColor);
	bool clearColorProfileCollectible();
	bool changeColorProfileIndex(uint8 index);
	bool clearColorProfileIndex();
	[[nodiscard]] DocumentId profileBackgroundEmojiId() const;
	bool changeProfileBackgroundEmojiId(DocumentId id);

	void setEmojiStatus(const MTPEmojiStatus &status);
	void setEmojiStatus(EmojiStatusId emojiStatusId, TimeId until = 0);
	[[nodiscard]] EmojiStatusId emojiStatusId() const;

	__declspec(dllexport) [[nodiscard]] bool isUser() const {
		return peerIsUser(id);
	}
	__declspec(dllexport) [[nodiscard]] bool isChat() const {
		return peerIsChat(id);
	}
	__declspec(dllexport) [[nodiscard]] bool isChannel() const {
		return peerIsChannel(id);
	}
	__declspec(dllexport) [[nodiscard]] bool isBot() const;
	__declspec(dllexport) [[nodiscard]] bool isSelf() const;
	__declspec(dllexport) [[nodiscard]] bool isVerified() const;
	__declspec(dllexport) [[nodiscard]] bool isPremium() const;
	__declspec(dllexport) [[nodiscard]] bool isScam() const;
	__declspec(dllexport) [[nodiscard]] bool isFake() const;
	__declspec(dllexport) [[nodiscard]] bool isMegagroup() const;
	__declspec(dllexport) [[nodiscard]] bool isBroadcast() const;
	__declspec(dllexport) [[nodiscard]] bool isForum() const;
	__declspec(dllexport) [[nodiscard]] bool isMonoforum() const;
	__declspec(dllexport) [[nodiscard]] bool isGigagroup() const;
	__declspec(dllexport) [[nodiscard]] bool isRepliesChat() const;
	__declspec(dllexport) [[nodiscard]] bool isVerifyCodes() const;
	__declspec(dllexport) [[nodiscard]] bool isFreezeAppealChat() const;
	__declspec(dllexport) [[nodiscard]] bool sharedMediaInfo() const;
	__declspec(dllexport) [[nodiscard]] bool savedSublistsInfo() const;
	__declspec(dllexport) [[nodiscard]] bool hasStoriesHidden() const;
	__declspec(dllexport) void setStoriesHidden(bool hidden);

	[[nodiscard]] Ui::BotVerifyDetails *botVerifyDetails() const;

	[[nodiscard]] bool isNotificationsUser() const {
		return (id == peerFromUser(333000))
			|| (id == kServiceNotificationsId);
	}
	[[nodiscard]] bool isServiceUser() const {
		return isUser() && !(id.value % 1000);
	}
	[[nodiscard]] bool isSavedHiddenAuthor() const {
		return (id == kSavedHiddenAuthorId);
	}

	[[nodiscard]] Data::Forum *forum() const;
	[[nodiscard]] Data::ForumTopic *forumTopicFor(MsgId rootId) const;

	[[nodiscard]] Data::SavedMessages *monoforum() const;
	[[nodiscard]] Data::SavedSublist *monoforumSublistFor(
		PeerId sublistPeerId) const;

	[[nodiscard]] bool useSubsectionTabs() const;
	[[nodiscard]] bool viewForumAsMessages() const;
	void processTopics(const MTPVector<MTPForumTopic> &topics);

	[[nodiscard]] Data::PeerNotifySettings &notify() {
		return _notify;
	}
	[[nodiscard]] const Data::PeerNotifySettings &notify() const {
		return _notify;
	}

	__declspec(dllexport) [[nodiscard]] bool isAyuNoForwards() const;
	__declspec(dllexport) [[nodiscard]] bool allowsForwarding() const;
	__declspec(dllexport) [[nodiscard]] Data::RestrictionCheckResult amRestricted(
		ChatRestriction right) const;
	__declspec(dllexport) [[nodiscard]] bool amAnonymous() const;
	__declspec(dllexport) [[nodiscard]] bool canRevokeFullHistory() const;
	__declspec(dllexport) [[nodiscard]] bool slowmodeApplied() const;
	__declspec(dllexport) [[nodiscard]] rpl::producer<bool> slowmodeAppliedValue() const;
	__declspec(dllexport) [[nodiscard]] int slowmodeSecondsLeft() const;
	__declspec(dllexport) [[nodiscard]] bool canManageGroupCall() const;
	__declspec(dllexport) [[nodiscard]] bool canManageRanks() const;

	__declspec(dllexport) [[nodiscard]] bool amMonoforumAdmin() const;

	__declspec(dllexport) [[nodiscard]] int starsPerMessage() const;
	__declspec(dllexport) [[nodiscard]] int starsPerMessageChecked() const;
	__declspec(dllexport) [[nodiscard]] Data::StarsRating starsRating() const;

	__declspec(dllexport) [[nodiscard]] UserData *asBot();
	__declspec(dllexport) [[nodiscard]] const UserData *asBot() const;
	__declspec(dllexport) [[nodiscard]] UserData *asUser();
	__declspec(dllexport) [[nodiscard]] const UserData *asUser() const;
	__declspec(dllexport) [[nodiscard]] ChatData *asChat();
	__declspec(dllexport) [[nodiscard]] const ChatData *asChat() const;
	__declspec(dllexport) [[nodiscard]] ChannelData *asChannel();
	__declspec(dllexport) [[nodiscard]] const ChannelData *asChannel() const;
	__declspec(dllexport) [[nodiscard]] ChannelData *asMegagroup();
	__declspec(dllexport) [[nodiscard]] const ChannelData *asMegagroup() const;
	__declspec(dllexport) [[nodiscard]] ChannelData *asBroadcast();
	__declspec(dllexport) [[nodiscard]] const ChannelData *asBroadcast() const;
	__declspec(dllexport) [[nodiscard]] ChatData *asChatNotMigrated();
	__declspec(dllexport) [[nodiscard]] const ChatData *asChatNotMigrated() const;
	__declspec(dllexport) [[nodiscard]] ChannelData *asChannelOrMigrated();
	__declspec(dllexport) [[nodiscard]] const ChannelData *asChannelOrMigrated() const;
	__declspec(dllexport) [[nodiscard]] ChannelData *asMonoforum();
	__declspec(dllexport) [[nodiscard]] const ChannelData *asMonoforum() const;

	[[nodiscard]] ChatData *migrateFrom() const;
	[[nodiscard]] ChannelData *migrateTo() const;
	[[nodiscard]] not_null<PeerData*> migrateToOrMe();
	[[nodiscard]] not_null<const PeerData*> migrateToOrMe() const;
	[[nodiscard]] not_null<PeerData*> userpicPaintingPeer();
	[[nodiscard]] not_null<const PeerData*> userpicPaintingPeer() const;
	[[nodiscard]] Ui::PeerUserpicShape userpicShape() const;

	// isMonoforum() ? monoforumLink() : nullptr
	[[nodiscard]] ChannelData *monoforumBroadcast() const;

	// isMonoforum() ? nullptr : monoforumLink()
	[[nodiscard]] ChannelData *broadcastMonoforum() const;

	void updateFull();
	void updateFullForced();
	void fullUpdated();
	[[nodiscard]] bool wasFullUpdated() const {
		return (_lastFullUpdate != 0);
	}

	__declspec(dllexport) [[nodiscard]] int nameVersion() const;
	__declspec(dllexport) [[nodiscard]] const QString &name() const;
	__declspec(dllexport) [[nodiscard]] const QString &shortName() const;
	__declspec(dllexport) [[nodiscard]] const QString &topBarNameText() const;

	__declspec(dllexport) [[nodiscard]] QString username() const;
	__declspec(dllexport) [[nodiscard]] QString editableUsername() const;
	__declspec(dllexport) [[nodiscard]] const std::vector<QString> &usernames() const;
	__declspec(dllexport) [[nodiscard]] bool isUsernameEditable(QString username) const;

	__declspec(dllexport) [[nodiscard]] const base::flat_set<QString> &nameWords() const {
		return _nameWords;
	}
	__declspec(dllexport) [[nodiscard]] const base::flat_set<QChar> &nameFirstLetters() const {
		return _nameFirstLetters;
	}

	void setUserpic(
		PhotoId photoId,
		const ImageLocation &location,
		bool hasVideo);
	void setUserpicPhoto(const MTPPhoto &data);

	void paintUserpic(
		QPainter &p,
		Ui::PeerUserpicView &view,
		PaintUserpicContext context) const;
	void paintUserpic(
			QPainter &p,
			Ui::PeerUserpicView &view,
			int x,
			int y,
			int size,
			bool forceCircle = false) const {
		paintUserpic(p, view, {
			.position = { x, y },
			.size = size,
			.shape = (forceCircle
				? Ui::PeerUserpicShape::Circle
				: Ui::PeerUserpicShape::Auto),
		});
	}
	void paintUserpicLeft(
			QPainter &p,
			Ui::PeerUserpicView &view,
			int x,
			int y,
			int w,
			int size,
			bool forceCircle = false) const {
		paintUserpic(
			p,
			view,
			rtl() ? (w - x - size) : x,
			y,
			size,
			forceCircle);
	}
	void loadUserpic();
	[[nodiscard]] bool hasUserpic() const;
	[[nodiscard]] Ui::PeerUserpicView activeUserpicView();
	[[nodiscard]] Ui::PeerUserpicView createUserpicView();
	[[nodiscard]] bool useEmptyUserpic(Ui::PeerUserpicView &view) const;
	[[nodiscard]] InMemoryKey userpicUniqueKey(Ui::PeerUserpicView &view) const;
	[[nodiscard]] static QImage GenerateUserpicImage(
		not_null<PeerData*> peer,
		Ui::PeerUserpicView &view,
		int size,
		std::optional<int> radius = {});
	[[nodiscard]] ImageLocation userpicLocation() const;

	static constexpr auto kUnknownPhotoId = PhotoId(0xFFFFFFFFFFFFFFFFULL);
	__declspec(dllexport) [[nodiscard]] bool userpicPhotoUnknown() const;
	__declspec(dllexport) [[nodiscard]] PhotoId userpicPhotoId() const;
	__declspec(dllexport) [[nodiscard]] bool userpicHasVideo() const;
	__declspec(dllexport) [[nodiscard]] Data::FileOrigin userpicOrigin() const;
	__declspec(dllexport) [[nodiscard]] Data::FileOrigin userpicPhotoOrigin() const;

	// If this string is not empty we must not allow to open the
	// conversation and we must show this string instead.
	[[nodiscard]] QString computeUnavailableReason() const;
	[[nodiscard]] bool hasSensitiveContent() const;
	void setUnavailableReasons(
		std::vector<Data::UnavailableReason> &&reason);

	[[nodiscard]] ClickHandlerPtr createOpenLink();
	[[nodiscard]] const ClickHandlerPtr &openLink() {
		if (!_openLink) {
			_openLink = createOpenLink();
		}
		return _openLink;
	}

	[[nodiscard]] QImage *userpicCloudImage(Ui::PeerUserpicView &view) const;

	__declspec(dllexport) [[nodiscard]] bool canPinMessages() const;
	__declspec(dllexport) [[nodiscard]] bool canEditMessagesIndefinitely() const;
	__declspec(dllexport) [[nodiscard]] bool canCreatePolls() const;
	__declspec(dllexport) [[nodiscard]] bool canCreateTodoLists() const;
	__declspec(dllexport) [[nodiscard]] bool canCreateTopics() const;
	__declspec(dllexport) [[nodiscard]] bool canManageTopics() const;
	__declspec(dllexport) [[nodiscard]] bool canPostStories() const;
	__declspec(dllexport) [[nodiscard]] bool canEditStories() const;
	__declspec(dllexport) [[nodiscard]] bool canDeleteStories() const;
	__declspec(dllexport) [[nodiscard]] bool canManageGifts() const;
	__declspec(dllexport) [[nodiscard]] bool canTransferGifts() const;
	__declspec(dllexport) [[nodiscard]] bool canExportChatHistory() const;
	__declspec(dllexport) [[nodiscard]] bool autoTranslation() const;

	// Returns true if about text was changed.
	__declspec(dllexport) bool setAbout(const QString &newAbout);
	__declspec(dllexport) [[nodiscard]] const QString &about() const {
		return _about;
	}

	void checkFolder(FolderId folderId);

	void setBarSettings(PeerBarSettings which);
	[[nodiscard]] auto barSettings() const {
		return (_barSettings.current() & PeerBarSetting::Unknown)
			? std::nullopt
			: std::make_optional(_barSettings.current());
	}
	[[nodiscard]] auto barSettingsValue() const {
		return (_barSettings.current() & PeerBarSetting::Unknown)
			? _barSettings.changes()
			: (_barSettings.value() | rpl::type_erased);
	}
	__declspec(dllexport) [[nodiscard]] int paysPerMessage() const;
	__declspec(dllexport) void clearPaysPerMessage();
	__declspec(dllexport) [[nodiscard]] bool hideLinks() const;
	__declspec(dllexport) [[nodiscard]] QString requestChatTitle() const;
	__declspec(dllexport) [[nodiscard]] TimeId requestChatDate() const;
	__declspec(dllexport) [[nodiscard]] UserData *businessBot() const;
	__declspec(dllexport) [[nodiscard]] QString businessBotManageUrl() const;
	__declspec(dllexport) void clearBusinessBot();
	__declspec(dllexport) [[nodiscard]] QString phoneCountryCode() const;
	__declspec(dllexport) [[nodiscard]] int registrationMonth() const;
	__declspec(dllexport) [[nodiscard]] int registrationYear() const;
	__declspec(dllexport) [[nodiscard]] TimeId nameChangeDate() const;
	__declspec(dllexport) [[nodiscard]] TimeId photoChangeDate() const;

	enum class TranslationFlag : uchar {
		Unknown,
		Disabled,
		Enabled,
	};
	void setTranslationDisabled(bool disabled);
	[[nodiscard]] TranslationFlag translationFlag() const;
	void saveTranslationDisabled(bool disabled);

	void setBarSettings(const MTPPeerSettings &data);
	bool changeBackgroundEmojiId(
		const tl::conditional<MTPlong> &cloudBackgroundEmoji);
	bool changeColorCollectible(
		const tl::conditional<MTPPeerColor> &cloudColor);
	bool changeColor(const tl::conditional<MTPPeerColor> &cloudColor);
	bool changeColorProfile(const tl::conditional<MTPPeerColor> &cloudColor);

	enum class BlockStatus : char {
		Unknown,
		Blocked,
		NotBlocked,
	};
	__declspec(dllexport) [[nodiscard]] BlockStatus blockStatus() const {
		return _blockStatus;
	}
	__declspec(dllexport) [[nodiscard]] bool isBlocked() const {
		return (blockStatus() == BlockStatus::Blocked);
	}
	void setIsBlocked(bool is);

	enum class LoadedStatus : char {
		Not,
		Minimal,
		Normal,
		Full,
	};
	__declspec(dllexport) [[nodiscard]] LoadedStatus loadedStatus() const {
		return _loadedStatus;
	}
	[[nodiscard]] bool isMinimalLoaded() const {
		return (loadedStatus() != LoadedStatus::Not);
	}
	[[nodiscard]] bool isLoaded() const {
		return (loadedStatus() == LoadedStatus::Normal) || isFullLoaded();
	}
	[[nodiscard]] bool isFullLoaded() const {
		return (loadedStatus() == LoadedStatus::Full);
	}
	void setLoadedStatus(LoadedStatus status);

	__declspec(dllexport) [[nodiscard]] TimeId messagesTTL() const;
	__declspec(dllexport) void setMessagesTTL(TimeId period);

	[[nodiscard]] Data::GroupCall *groupCall() const;
	[[nodiscard]] PeerId groupCallDefaultJoinAs() const;

	__declspec(dllexport) void setThemeToken(const QString &token);
	__declspec(dllexport) [[nodiscard]] const QString &themeToken() const;

	void setWallPaper(
		std::optional<Data::WallPaper> paper,
		bool overriden = false);
	[[nodiscard]] bool wallPaperOverriden() const;
	[[nodiscard]] const Data::WallPaper *wallPaper() const;

	enum class StoriesState {
		Unknown,
		None,
		HasRead,
		HasUnread,
		HasVideoStream,
	};
	__declspec(dllexport) [[nodiscard]] bool hasActiveStories() const;
	__declspec(dllexport) [[nodiscard]] bool hasUnreadStories() const;
	__declspec(dllexport) [[nodiscard]] bool hasActiveVideoStream() const;
	__declspec(dllexport) void setStoriesState(StoriesState state);

	__declspec(dllexport) [[nodiscard]] int peerGiftsCount() const;

	[[nodiscard]] MTPInputPeer input() const;

	const PeerId id;

protected:
	void updateNameDelayed(
		const QString &newName,
		const QString &newNameOrPhone,
		const QString &newUsername);
	void updateUserpic(PhotoId photoId, MTP::DcId dcId, bool hasVideo);
	void clearUserpic();
	void invalidateEmptyUserpic();
	void checkTrustedPayForMessage();

private:
	void fillNames();
	[[nodiscard]] not_null<Ui::EmptyUserpic*> ensureEmptyUserpic() const;
	[[nodiscard]] virtual auto unavailableReasons() const
		-> const std::vector<Data::UnavailableReason> &;

	void setUserpicChecked(
		PhotoId photoId,
		const ImageLocation &location,
		bool hasVideo);

	virtual void setUnavailableReasonsList(
		std::vector<Data::UnavailableReason> &&reasons);
	void setHasSensitiveContent(bool has);

	const not_null<Data::Session*> _owner;

	mutable Data::CloudImage _userpic;
	PhotoId _userpicPhotoId = kUnknownPhotoId;

	mutable std::unique_ptr<Ui::EmptyUserpic> _userpicEmpty;

	Data::PeerNotifySettings _notify;

	ClickHandlerPtr _openLink;
	base::flat_set<QString> _nameWords; // for filtering
	base::flat_set<QChar> _nameFirstLetters;

	EmojiStatusId _emojiStatusId;
	DocumentId _backgroundEmojiId = 0;
	DocumentId _profileBackgroundEmojiId = 0;
	crl::time _lastFullUpdate = 0;

	QString _name;
	uint32 _nameVersion : 16 = 1;
	uint32 _sensitiveContent : 1 = 0;
	uint32 _wallPaperOverriden : 1 = 0;
	uint32 _checkedTrustedPayForMessage : 1 = 0;

	TimeId _ttlPeriod = 0;

	BarSettings _barSettings = PeerBarSettings(PeerBarSetting::Unknown);
	std::unique_ptr<PeerBarDetails> _barDetails;
	std::shared_ptr<Ui::ColorCollectible> _colorCollectible;
	std::shared_ptr<Ui::ColorCollectible> _colorProfileCollectible;

	BlockStatus _blockStatus = BlockStatus::Unknown;
	LoadedStatus _loadedStatus = LoadedStatus::Not;
	TranslationFlag _translationFlag = TranslationFlag::Unknown;
	uint8 _colorIndex : 6 = 0;
	uint8 _colorIndexCloud : 1 = 0;
	std::optional<uint8> _colorProfileIndex;
	uint8 _userpicHasVideo : 1 = 0;

	QString _about;
	QString _themeToken;
	std::unique_ptr<Data::WallPaper> _wallPaper;

};

namespace Data {

void SetTopPinnedMessageId(
	not_null<PeerData*> peer,
	MsgId messageId);
[[nodiscard]] FullMsgId ResolveTopPinnedId(
	not_null<PeerData*> peer,
	MsgId topicRootId,
	PeerId monoforumPeerId,
	PeerData *migrated = nullptr);
[[nodiscard]] FullMsgId ResolveMinPinnedId(
	not_null<PeerData*> peer,
	MsgId topicRootId,
	PeerId monoforumPeerId,
	PeerData *migrated = nullptr);

[[nodiscard]] uint64 BackgroundEmojiIdFromColor(const MTPPeerColor *color);
[[nodiscard]] std::optional<uint8> ColorIndexFromColor(const MTPPeerColor *);

[[nodiscard]] bool IsBotUserCreatesTopics(not_null<PeerData*>);

} // namespace Data
