/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "core/credits_amount.h"
#include "data/components/credits.h"
#include "data/data_birthday.h"
#include "data/data_peer.h"
#include "data/data_chat_participant_status.h"
#include "data/data_lastseen_status.h"
#include "data/data_user_names.h"
#include "dialogs/dialogs_key.h"
#include "base/flags.h"

namespace Data {
class Forum;
struct BotCommand;
struct BusinessDetails;
} // namespace Data

namespace Api {
enum class DisallowedGiftType : uchar;
using DisallowedGiftTypes = base::flags<DisallowedGiftType>;
} // namespace Api

struct StarRefProgram {
	CreditsAmount revenuePerUser;
	TimeId endDate = 0;
	ushort commission = 0;
	uint8 durationMonths = 0;

	friend inline constexpr bool operator==(
		StarRefProgram,
		StarRefProgram) = default;
};

struct BotVerifierSettings {
	DocumentId iconId = 0;
	QString company;
	QString customDescription;
	bool canModifyDescription = false;

	explicit operator bool() const {
		return iconId != 0;
	}

	friend inline bool operator==(
		const BotVerifierSettings &a,
		const BotVerifierSettings &b) = default;
};

struct BotInfo {
	enum class SetBotPhotoOpenState : uchar {
		Unknown,
		OpenedWithHistory,
		OpenedEmpty,
	};

	BotInfo();
	~BotInfo();

	void ensureForum(not_null<UserData*> that);
	[[nodiscard]] Data::Forum *forum() const;
	[[nodiscard]] std::unique_ptr<Data::Forum> takeForumData();

	QString description;
	QString inlinePlaceholder;
	std::vector<Data::BotCommand> commands;

	PhotoData *photo = nullptr;
	DocumentData *document = nullptr;

	QString botMenuButtonText;
	QString botMenuButtonUrl;
	QString privacyPolicyUrl;

	QColor botAppColorTitleDay = QColor(0, 0, 0, 0);
	QColor botAppColorTitleNight = QColor(0, 0, 0, 0);
	QColor botAppColorBodyDay = QColor(0, 0, 0, 0);
	QColor botAppColorBodyNight = QColor(0, 0, 0, 0);

	QString startToken;
	Dialogs::EntryState inlineReturnTo;

	ChatAdminRights groupAdminRights;
	ChatAdminRights channelAdminRights;

	StarRefProgram starRefProgram;
	std::unique_ptr<BotVerifierSettings> verifierSettings;

	int version = 0;
	int descriptionVersion = 0;
	int activeUsers = 0;
	SetBotPhotoOpenState setBotPhotoOpenState = SetBotPhotoOpenState::Unknown;
	bool inited : 1 = false;
	bool readsAllHistory : 1 = false;
	bool cantJoinGroups : 1 = false;
	bool supportsAttachMenu : 1 = false;
	bool canEditInformation : 1 = false;
	bool canManageEmojiStatus : 1 = false;
	bool supportsBusiness : 1 = false;
	bool hasMainApp : 1 = false;
	bool userCreatesTopics : 1 = false;
	bool setBotPhotoHidden : 1 = false;
	bool canManageBots : 1 = false;

private:
	std::unique_ptr<Data::Forum> _forum;

};

enum class UserDataFlag : uint32 {
	Contact = (1 << 0),
	MutualContact = (1 << 1),
	Deleted = (1 << 2),
	Verified = (1 << 3),
	Scam = (1 << 4),
	Fake = (1 << 5),
	BotInlineGeo = (1 << 6),
	Blocked = (1 << 7),
	HasPhoneCalls = (1 << 8),
	PhoneCallsPrivate = (1 << 9),
	Support = (1 << 10),
	CanPinMessages = (1 << 11),
	DiscardMinPhoto = (1 << 12),
	Self = (1 << 13),
	Premium = (1 << 14),
	UnofficialSecurityRisk = (1 << 15),
	VoiceMessagesForbidden = (1 << 16),
	PersonalPhoto = (1 << 17),
	StoriesHidden = (1 << 18),
	HasActiveStories = (1 << 19),
	HasUnreadStories = (1 << 20),
	RequiresPremiumToWrite = (1 << 21),
	HasRequirePremiumToWrite = (1 << 22),
	HasStarsPerMessage = (1 << 23),
	MessageMoneyRestrictionsKnown = (1 << 24),
	ReadDatesPrivate = (1 << 25),
	StoriesCorrespondent = (1 << 26),
	Forum = (1 << 27),
	HasActiveVideoStream = (1 << 28),
	NoForwardsMyEnabled = (1 << 29),
	NoForwardsPeerEnabled = (1 << 30),
};
inline constexpr bool is_flag_type(UserDataFlag) { return true; };
using UserDataFlags = base::flags<UserDataFlag>;

[[nodiscard]] Data::LastseenStatus LastseenFromMTP(
	const MTPUserStatus &status,
	Data::LastseenStatus currentStatus);

class UserData final : public PeerData {
public:
	using Flag = UserDataFlag;
	using Flags = Data::Flags<UserDataFlags>;

	UserData(not_null<Data::Session*> owner, PeerId id);
	~UserData();

	__declspec(dllexport) void setPhoto(const MTPUserProfilePhoto &photo);

	__declspec(dllexport) void setName(
		const QString &newFirstName,
		const QString &newLastName,
		const QString &newPhoneName,
		const QString &newUsername);
	__declspec(dllexport) void setUsernames(const Data::Usernames &newUsernames);

	__declspec(dllexport) void setUsername(const QString &username);
	__declspec(dllexport) void setPhone(const QString &newPhone);
	__declspec(dllexport) void setBotInfoVersion(int version);
	__declspec(dllexport) void setBotInfo(const MTPBotInfo &info);

	__declspec(dllexport) void setNameOrPhone(const QString &newNameOrPhone);

	__declspec(dllexport) void madeAction(TimeId when); // pseudo-online

	__declspec(dllexport) uint64 accessHash() const {
		return _accessHash;
	}
	__declspec(dllexport) void setAccessHash(uint64 accessHash);

	__declspec(dllexport) auto flags() const {
		return _flags.current();
	}
	__declspec(dllexport) auto flagsValue() const {
		return _flags.value();
	}
	__declspec(dllexport) void setFlags(UserDataFlags which);
	__declspec(dllexport) void addFlags(UserDataFlags which);
	__declspec(dllexport) void removeFlags(UserDataFlags which);

	__declspec(dllexport) [[nodiscard]] bool isVerified() const;
	__declspec(dllexport) [[nodiscard]] bool isScam() const;
	__declspec(dllexport) [[nodiscard]] bool isFake() const;
	__declspec(dllexport) [[nodiscard]] bool isPremium() const;
	__declspec(dllexport) [[nodiscard]] bool isBotInlineGeo() const;
	__declspec(dllexport) [[nodiscard]] bool isBot() const;
	__declspec(dllexport) [[nodiscard]] bool isSupport() const;
	__declspec(dllexport) [[nodiscard]] bool isInaccessible() const;
	__declspec(dllexport) [[nodiscard]] bool applyMinPhoto() const;
	__declspec(dllexport) [[nodiscard]] bool hasPersonalPhoto() const;
	__declspec(dllexport) [[nodiscard]] bool hasStoriesHidden() const;
	__declspec(dllexport) [[nodiscard]] bool hasRequirePremiumToWrite() const;
	__declspec(dllexport) [[nodiscard]] bool hasStarsPerMessage() const;
	__declspec(dllexport) [[nodiscard]] bool requiresPremiumToWrite() const;
	__declspec(dllexport) [[nodiscard]] bool messageMoneyRestrictionsKnown() const;
	__declspec(dllexport) [[nodiscard]] bool canSendIgnoreMoneyRestrictions() const;
	__declspec(dllexport) [[nodiscard]] bool readDatesPrivate() const;
	__declspec(dllexport) [[nodiscard]] bool allowsForwarding() const;
	__declspec(dllexport) [[nodiscard]] bool isAyuNoForwards() const;
	__declspec(dllexport) void setNoForwardsFlags(bool myEnabled, bool peerEnabled);
	[[nodiscard]] bool isForum() const {
		return flags() & Flag::Forum;
	}
	[[nodiscard]] Data::Forum *forum() const {
		return botInfo ? botInfo->forum() : nullptr;
	}

	__declspec(dllexport) void setStoriesCorrespondent(bool is);
	__declspec(dllexport) [[nodiscard]] bool storiesCorrespondent() const;

	__declspec(dllexport) void setStarsPerMessage(int stars);
	__declspec(dllexport) [[nodiscard]] int starsPerMessage() const;

	__declspec(dllexport) void setStarsRating(Data::StarsRating value);
	__declspec(dllexport) [[nodiscard]] Data::StarsRating starsRating() const;

	__declspec(dllexport) [[nodiscard]] bool canShareThisContact() const;
	__declspec(dllexport) [[nodiscard]] bool canAddContact() const;

	// In Data::Session::processUsers() we check only that.
	// When actually trying to share contact we perform
	// a full check by canShareThisContact() call.
	__declspec(dllexport) [[nodiscard]] bool canShareThisContactFast() const;

	__declspec(dllexport) [[nodiscard]] const QString &phone() const;
	__declspec(dllexport) [[nodiscard]] QString username() const;
	__declspec(dllexport) [[nodiscard]] QString editableUsername() const;
	__declspec(dllexport) [[nodiscard]] const std::vector<QString> &usernames() const;
	__declspec(dllexport) [[nodiscard]] bool isUsernameEditable(QString username) const;

	__declspec(dllexport) void setBotVerifyDetails(Ui::BotVerifyDetails details);
	__declspec(dllexport) void setBotVerifyDetailsIcon(DocumentId iconId);
	__declspec(dllexport) [[nodiscard]] Ui::BotVerifyDetails *botVerifyDetails() const {
		return _botVerifyDetails.get();
	}

	enum class ContactStatus : char {
		Unknown,
		Contact,
		NotContact,
	};
	__declspec(dllexport) [[nodiscard]] ContactStatus contactStatus() const;
	__declspec(dllexport) [[nodiscard]] bool isContact() const;
	__declspec(dllexport) void setIsContact(bool is);

	__declspec(dllexport) [[nodiscard]] Data::LastseenStatus lastseen() const;
	__declspec(dllexport) bool updateLastseen(Data::LastseenStatus value);

	enum class CallsStatus : char {
		Unknown,
		Enabled,
		Disabled,
		Private,
	};
	__declspec(dllexport) CallsStatus callsStatus() const;
	__declspec(dllexport) bool hasCalls() const;
	__declspec(dllexport) void setCallsStatus(CallsStatus callsStatus);

	__declspec(dllexport) [[nodiscard]] Data::Birthday birthday() const;
	__declspec(dllexport) void setBirthday(Data::Birthday value);
	__declspec(dllexport) void setBirthday(const tl::conditional<MTPBirthday> &value);

	__declspec(dllexport) [[nodiscard]] int commonChatsCount() const;
	__declspec(dllexport) void setCommonChatsCount(int count);

	__declspec(dllexport) [[nodiscard]] int peerGiftsCount() const;
	__declspec(dllexport) void setPeerGiftsCount(int count);

	__declspec(dllexport) [[nodiscard]] bool hasPrivateForwardName() const;
	__declspec(dllexport) [[nodiscard]] QString privateForwardName() const;
	__declspec(dllexport) void setPrivateForwardName(const QString &name);

	__declspec(dllexport) [[nodiscard]] bool hasActiveStories() const;
	__declspec(dllexport) [[nodiscard]] bool hasUnreadStories() const;
	__declspec(dllexport) [[nodiscard]] bool hasActiveVideoStream() const;
	__declspec(dllexport) void setStoriesState(StoriesState state);

	__declspec(dllexport) [[nodiscard]] const Data::BusinessDetails &businessDetails() const;
	__declspec(dllexport) void setBusinessDetails(Data::BusinessDetails details);

	__declspec(dllexport) void setStarRefProgram(StarRefProgram program);

	__declspec(dllexport) [[nodiscard]] ChannelId personalChannelId() const;
	__declspec(dllexport) [[nodiscard]] MsgId personalChannelMessageId() const;
	__declspec(dllexport) void setPersonalChannel(ChannelId channelId, MsgId messageId);

	[[nodiscard]] UserId botManagerId() const;
	void setBotManagerId(UserId managerId);

	[[nodiscard]] bool unofficialSecurityRisk() const {
		return flags() & Flag::UnofficialSecurityRisk;
	}

	[[nodiscard]] MTPInputUser inputUser() const;

	QString firstName;
	QString lastName;
	QString nameOrPhone;

	std::unique_ptr<BotInfo> botInfo;

	__declspec(dllexport) [[nodiscard]] Api::DisallowedGiftTypes disallowedGiftTypes() const {
		return _disallowedGiftTypes;
	}
	__declspec(dllexport) void setDisallowedGiftTypes(Api::DisallowedGiftTypes types);

	[[nodiscard]] const TextWithEntities &note() const;
	__declspec(dllexport) void setNote(const TextWithEntities &note);

private:
	auto unavailableReasons() const
		-> const std::vector<Data::UnavailableReason> & override;

	void setUnavailableReasonsList(
		std::vector<Data::UnavailableReason> &&reasons) override;

	Flags _flags;
	Data::LastseenStatus _lastseen;
	Data::Birthday _birthday;
	int _commonChatsCount = 0;
	int _peerGiftsCount = 0;
	int _starsPerMessage = 0;
	ContactStatus _contactStatus = ContactStatus::Unknown;
	CallsStatus _callsStatus = CallsStatus::Unknown;

	Data::UsernamesInfo _username;

	std::unique_ptr<Data::BusinessDetails> _businessDetails;
	std::vector<Data::UnavailableReason> _unavailableReasons;
	QString _phone;
	QString _privateForwardName;
	std::unique_ptr<Ui::BotVerifyDetails> _botVerifyDetails;
	Data::StarsRating _starsRating;

	ChannelId _personalChannelId = 0;
	MsgId _personalChannelMessageId = 0;
	UserId _botManagerId = 0;

	uint64 _accessHash = 0;
	static constexpr auto kInaccessibleAccessHashOld
		= 0xFFFFFFFFFFFFFFFFULL;

	Api::DisallowedGiftTypes _disallowedGiftTypes;
	TextWithEntities _note;

};

namespace Data {

void ApplyUserUpdate(not_null<UserData*> user, const MTPDuserFull &update);

[[nodiscard]] StarRefProgram ParseStarRefProgram(
	const MTPStarRefProgram *program);

[[nodiscard]] Ui::BotVerifyDetails ParseBotVerifyDetails(
	const MTPBotVerification *info);

} // namespace Data
