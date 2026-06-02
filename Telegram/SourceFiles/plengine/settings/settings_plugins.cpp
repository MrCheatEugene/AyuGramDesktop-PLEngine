/*
Emil Kh, AKA Pomorgite - t.me/Pomorgite // pmrgt.com
AyuGram Plugin engine, 2026 // t.me/ayuplugg
Follows GNU GPL v3 and Telegram Desktop licensing.

This part of source code is based on Ayugram's sources by Radolyn.
*/

#include "plengine/settings/settings_plugins.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "settings/sections/settings_main.h"
#include "lang_auto.h"
#include "plengine/settings/settings_main.h"
#include "ayu/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "ExternSharedVariables.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QLayout>

namespace Settings {

using namespace Builder;
using namespace Settings::AyuBuilder;

namespace {

void cleanLayout(SectionBuilder builder) {
	for (auto w : builder.container()->children()) {
		if (w->isWidgetType()) {
			delete w;
		}
	}
	auto oldLayout = builder.container()->layout();
	if (oldLayout) delete oldLayout;
}

SectionBuilder secBuilder(SectionBuilder builder) {
	auto newLayout = new QVBoxLayout();
	newLayout->setContentsMargins(0, 0, 0, 0);
	builder.container()->setLayout(newLayout);
	auto content = Ui::CreateChild<Ui::VerticalLayout>(builder.container());
	newLayout->addWidget(content);
	SectionBuilder sb(WidgetContext(content, builder.controller()));
	return sb;
}

void saveLayout(SectionBuilder builder, SectionBuilder sb) {
	builder.container()->layout()->update();
	Ui::ResizeFitChild(builder.container(), sb.container());
	builder.container()->update();
}

void BuildCategories(SectionBuilder &builder, AyuSectionBuilder &ayu, PLEPlugins* ple) {
	builder.addSkip();
	auto breakEveryN = [](const QString& input, int n) {
		QString result;
		for (int i = 0; i < input.size(); i += n) {
			if (i > 0) result += '\n';
			result += input.mid(i, n);
		}
		return result.trimmed();
		};

	for (size_t i = 0; i < pluginsData.size(); i++) {
		auto x = pluginsData.at(i);
		builder.addDivider();
		builder.addButton({
			.title = rpl::single(breakEveryN(QString::fromStdString(x.name), 32)),
			.icon = x.usesHooks ? &st::menuIconEdit : &st::menuIconExperimental,
			.label = rpl::single(QString(x.usesHooks ? "Uses hooks" : "")),
			.onClick = [=] {
				auto msg = QString::fromStdString("DLL: " + x.modulePath + "\n\nUses hooks:\n" + x.hooksList);
				if (x.drawGUI != nullptr) {
					cleanLayout(builder);
					auto sb = secBuilder(builder);
					auto _ayu = AyuSectionBuilder(sb);
					x.drawGUI(sb, _ayu, ple);
					sb.addSkip();
					sb.addDivider();
					sb.addButton({
						.title = rpl::single(QString("Back to loaded plugins")),
						.icon = { &st::menuIconExperimental },
						.onClick = [=]{
							cleanLayout(builder);
							auto __sb = secBuilder(builder);
							auto _ayu = AyuSectionBuilder(__sb);
							BuildCategories(__sb, _ayu, ple);
							saveLayout(builder, __sb);
						}
					});
					sb.addSkip();
					sb.addDividerText(rpl::single("UI above is managed by the plugin.\n" +msg));
					sb.addDivider();
					sb.addSkip();
					saveLayout(builder, sb);
					return;
				}
				else {
					builder.controller()->showToast(breakEveryN(msg, 42));
				}
			}
			});
		builder.addDividerText(rpl::single(breakEveryN(QString::fromStdString(x.description), 42)));
		builder.addSkip();
	}
}


} // namespace

rpl::producer<QString> PLEPlugins::title() {
	return rpl::single(QString(""));
}

PLEPlugins::PLEPlugins(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void PLEPlugins::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	auto kMeta = BuildHelper({
		.id = PLEPlugins::Id(),
		.parentId = MainId(),
		.title = &tr::ayu_AyuPreferences,
		.icon = &st::menuIconPremium,
		}, [this](SectionBuilder& builder) {
			auto ayu = AyuSectionBuilder(builder);
			
			BuildCategories(builder, ayu, this);
		});

	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type PLEPluginsId() {
	return PLEPlugins::Id();
}

} // namespace Settings
