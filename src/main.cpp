#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <alphalaneous.alphas_geode_utils/include/ObjectModify.hpp>
#include <alphalaneous.alphas_geode_utils/include/Utils.hpp>

#include <geode.texture-loader/include/TextureLoader.hpp>

//typedef geode::texture_loader::Pack Pack; -- impostor
struct PackInfo {
    VersionInfo m_textureldr;
    std::string m_id;
    std::string m_name;
    VersionInfo m_version;
    std::vector<std::string> m_authors;

    static Result<PackInfo> from(matjson::Value const& json);
};
class Pack {
public:
    std::filesystem::path m_path;
    std::filesystem::path m_unzippedPath;
    std::filesystem::path m_resourcesPath;
    std::optional<PackInfo> m_info;
};

class PackSelectPopup; //predecl for PackNode
class PackNode : public CCNode {
public:
    PackSelectPopup* m_layer;
    std::shared_ptr<Pack> m_pack;
    NineSlice* m_draggingBg;
};
class $baseModify(PackNodeExt, PackNode) {
	void onPackName(CCObject*) {
        auto tempPack = CCTexturePack();
		tempPack.m_paths.push_back(string::pathToString(m_pack->m_resourcesPath));
        tempPack.m_id = "Temp for " + string::pathToString(m_pack->m_path.filename());
		CCFileUtils::get()->addTexturePack(tempPack);
		CCFileUtils::get()->updatePaths();

		auto filenames = std::vector<std::string>{
			"menuloop.mp3",
			"GJ_gradientBG.png",
			"square01_001.png",
			"bigFont.fnt",
			"bigFont.png",
			"goldFont.fnt",
			"goldFont.png",
			"GJ_button_01.png",
			"GJ_GameSheet03.plist"
		};
        for (auto name : filenames) {
            CCFileUtils::get()->m_fullPathCache.erase(name);
        }

        CCSpriteFrameCache::get()->removeSpriteFramesFromFile("GJ_GameSheet03.plist"); //remove loaded
        CCSpriteFrameCache::get()->addSpriteFramesWithFile("GJ_GameSheet03.plist"); //add temp ones

        auto inf = std::stringstream();
        auto xd = false GEODE_WINDOWS(+1) ? "" : "file://";
        inf << fmt::format("## [{0}]({1}{0})", string::pathToString(m_pack->m_path), xd) << "\n";
        inf << fmt::format("- Unzipped at: [{0}]({1}{0})", string::pathToString(m_pack->m_unzippedPath), xd) << "\n";
        inf << fmt::format("- Search path at: [{0}]({1}{0})", string::pathToString(m_pack->m_resourcesPath), xd) << "\n";
        if (m_pack->m_info.has_value()) {
            auto info = &m_pack->m_info.value();
            inf << "## Pack info" << "\n";
			inf << "- Textureldr ver: " << info->m_textureldr.toVString() << "\n";
			inf << "- ID: " << info->m_id << "\n";
			inf << "- Name: " << info->m_name << "\n";
			inf << "- Version: " << info->m_version.toVString() << "\n";
            inf << "- Authors: ";
            for (auto a : info->m_authors) inf << a << (info->m_authors.size() > 2 ? ", " : "");
            inf << "\n";
        }
		Ref popup = MDPopup::create(
            string::pathToString(m_pack->m_path.filename()),
            inf.str().c_str(),
			"OK"
        );
        addSideArt(popup);
        popup->m_noElasticity = true;
        popup->addChild(geode::createLayerBG(), -10);

        if (auto a = popup->m_mainLayer->getChildByType<CCLabelBMFont>(0)) a->setFntFile("bigFont.fnt");

        findFirstChildRecursive<CCLabelBMFont>( // fuck youuu ewww
            popup->m_mainLayer, [](CCLabelBMFont* label) -> bool {
                if (string::contains(label->getFntFile(), "/")) return !"there is a bug updating md text area..";
                auto a = CCFileUtils::get()->fullPathForFilename(label->m_sFntFile.c_str(), 0);
                label->setFntFile(a.c_str());
                return false;
            }
        );

		GameManager::get()->fadeInMusic("menuloop.mp3");
        popup->addOnExitCallback([] { GameManager::get()->fadeInMenuMusic(); });

        popup->show();

        CCSpriteFrameCache::get()->removeSpriteFramesFromFile("GJ_GameSheet03.plist"); //remove loaded
        CCSpriteFrameCache::get()->removeSpriteFramesFromFile("GJ_GameSheet04.plist"); //remove loaded

        for (auto name : filenames) CCFileUtils::get()->m_fullPathCache.erase(name);
        CCFileUtils::get()->removeTexturePack(tempPack.m_id);

        CCSpriteFrameCache::get()->addSpriteFramesWithFile("GJ_GameSheet03.plist"); //restore
        CCSpriteFrameCache::get()->addSpriteFramesWithFile("GJ_GameSheet04.plist"); //restore
	}
    void modify() {
        auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(querySelector("pack-name-button"));
		if (!button) return log::error("`pack-name-button` in {} is {}", this, button);
        else {
			button->setEnabled(true);
			button->setTarget(this, menu_selector(PackNodeExt::onPackName));
        }
    }
};

class PackSelectPopup : public Popup {
public:
    ScrollLayer* m_availableList = nullptr;
    ScrollLayer* m_appliedList = nullptr;
    CCLabelBMFont* m_infoLabel = nullptr;
    PackNode* m_draggingNode = nullptr;
    size_t m_lastDragIdx = size_t(-1);
    int m_dragListFrom, m_dragListTo; //Available 0, Applied 1
};
class $baseModify(PackSelectPopupExt, PackSelectPopup) {
    static void reloadPacks() {
		auto popup = CCScene::get()->getChildByType<PackSelectPopup>(0);
		if (!popup) return log::error("No pack select popup found to reload");
		auto item = typeinfo_cast<CCMenuItem*>(popup->querySelector("reload-button"));
		if (!item) return log::error("`reload-button` in {} is {}", popup, item);
		item->activate();
	}
    static auto onCreatePack() {
        std::vector<CCNode*> xd;
        //id
        Ref ID = TextInput::create(262.000f, "me.my-resource-pack");
        ID->setPosition({ 230.000f, 190.000f });
        ID->getInputNode()->addChild(SimpleTextArea::create("ID:\n \n \n ", "bigFont.fnt", 0.5f, 262.000f), 0, 100);
        ID->getInputNode()->m_allowedChars = "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
		xd.push_back(ID);
        //Name
        Ref Name = TextInput::create(324.000f, "My cool TP");
        Name->setPosition({ 200.000f, 136.000f });
        Name->getInputNode()->addChild(SimpleTextArea::create("Name:\n \n \n ", "bigFont.fnt", 0.5f, 324.000f), 0, 100);
        Name->getInputNode()->m_allowedChars = " !\"#$ % &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
		xd.push_back(Name);
        //Version
        Ref Version = TextInput::create(104.000f, "1.0.0");
        Version->setPosition({ 90.000f, 82.000f });
        Version->getInputNode()->addChild(SimpleTextArea::create("Version:\n \n \n ", "bigFont.fnt", 0.5f, 104.000f), 0, 100);
        Version->getInputNode()->m_allowedChars = "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
        xd.push_back(Version);
        //Authors
        Ref Authors = TextInput::create(214.000f, R"("Me", "My pal")");
        Authors->setPosition({ 254.000f, 82.000f });
        Authors->getInputNode()->addChild(SimpleTextArea::create("Authors:\n \n \n ", "bigFont.fnt", 0.5f, 214.000f), 0, 100);
        Authors->getInputNode()->m_allowedChars = " !\"#$ % &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        xd.push_back(Authors);
        //lazyspr
        Ref icon = CCSprite::create("geode.texture-loader/noLogo.png");
        limitNodeSize(icon, { 48.000f, 48.000f }, 999.f, 0.f);
		auto iconbtn = CCMenuItemExt::createSpriteExtra(
            icon, [icon](void*) {
#ifdef __clang__
                async::spawn(
                    file::pick(
                        file::PickMode::OpenFile, file::FilePickOptions {
                            .filters = { 
                                file::FilePickOptions::Filter {
                                    .description = "Logo icon",
                                    .files = { "*.png" },
                                }
                            }
                        }
                    ), [icon](Result<std::optional<std::filesystem::path>> result) {
						log::info("Got file pick result: {}", result);
                        if (result.isOk() && result.unwrap().has_value()) {
                            auto path = string::pathToString(std::move(result).unwrap().value());
                            icon->initWithFile(path.c_str());
                            icon->setID(path.c_str());
                        }
                        else if (!result.isOk()) {
                            FLAlertLayer::create("Unable to Select File", result.unwrapErr(), "OK")->show();
                        }
                        limitNodeSize(icon, { 48.000f, 48.000f }, 999.f, 0.f);
                    }
                );
#else
                FLAlertLayer::create("Unable to Select File", "Mod built using MSVC...", "OK")->show();
#endif
			}
        );
        iconbtn->setPosition({ 64.000f, 196.000f });
        xd.push_back(iconbtn);
        //create and showwww
        auto popup = MDPopup::create(
            "Create Texture Pack",
            " \n\n \n\n \n\n \n\n \n\n \n\n",
            "Cancel", "Create", [=](bool a) {
                if (!a) return; // Cancel
                auto mod = Loader::get()->getInstalledMod("geode.texture-loader");
				auto wd = mod->getConfigDir() / "packs" / ID->getString().c_str();
                std::error_code err;
                std::filesystem::create_directories(wd, err);
                //pack.json
                auto json = matjson::Value();
				json["textureldr"] = mod->getVersion().toNonVString().c_str();
				json["name"] = Name->getString().c_str();
                json["authors"] = matjson::parse("[" + std::string(Authors->getString().c_str()) + "]").unwrapOrDefault();
                if (!json["authors"].size() or !json["authors"].isArray()) {
                    auto authors = matjson::Value().array();
                    authors.push(Authors->getString().c_str());
                    json["authors"] = authors;
                }
				json["id"] = ID->getString().c_str();
				json["version"] = Version->getString().c_str();
                file::writeToJson(wd / "pack.json", json);
                //logo.png
                std::filesystem::copy_file(icon->getID().c_str(), wd / "pack.png", err);
                //xd
                reloadPacks();
            }
        );
        for (auto a : xd) popup->m_buttonMenu->addChild(a);
        popup->show();
    }
	void modify() {
        auto create = CCMenuItemExt::createSpriteExtraWithFrameName(
            "GJ_newBtn_001.png", 0.400f, [](void*) { onCreatePack(); }
        );
		create->setID("create-pack-button"_spr);
        create->setPosition({ 376.000f, 25.000f });
        this->m_buttonMenu->addChild(create);
	}
};