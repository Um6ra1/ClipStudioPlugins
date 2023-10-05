/**
 * @file	
 * @brief	色収差フィルタ
 * @author	Um6r41 (同志たつとでっかいの)
 * @date 2023.10.05
 */

#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#include "../TriglavPlugInSDK/TriglavPlugInSDK.h"
#include "../DBG/DBG.h"
#include "resource.h"
#include <vector>
#include <string>

#define rep(i,m) for(long i=0;i<m;i++)


#define BLEND_MUL(x, y)	(BYTE)((double)((x) * (y)) / 255.0)
#define BLEND_SCREEN(x, y)	(BYTE)(255.0 - (double)((255 - (x)) * (255 - (y))) / 255.0)

const char* APP_NAME = "Posterisierung";

// 各プラグインで唯一でなければならず、重複しないようにする
const std::string MODULE_ID = "DEADBEEF-0000-0000-0000-000077770002";

const int MIN_TONES = 2;
const int MAX_TONES = 32;
const int RGB_DIM = 3;

void CreateSlider(TriglavPlugInServer* pluginServer, TriglavPlugInPropertyObject propertyObject, int captionID, int itemID, int min, int max, int defValue, int initValue, char shortcutKey) {
	TriglavPlugInStringObject caption = NULL;
	auto pStringService = pluginServer->serviceSuite.stringService;
	auto pPropertyService = pluginServer->serviceSuite.propertyService;

	pStringService->createWithStringIDProc(&caption, captionID, pluginServer->hostObject);
	pPropertyService->addItemProc(propertyObject, itemID, kTriglavPlugInPropertyValueTypeInteger,
		kTriglavPlugInPropertyValueKindDefault, kTriglavPlugInPropertyInputKindDefault, caption, shortcutKey);
	pPropertyService->setIntegerValueProc(propertyObject, itemID, initValue);
	pPropertyService->setIntegerDefaultValueProc(propertyObject, itemID, defValue);
	pPropertyService->setIntegerMinValueProc(propertyObject, itemID, min);
	pPropertyService->setIntegerMaxValueProc(propertyObject, itemID, max);
	pStringService->releaseProc(caption);
}


class MyFilter {
	int tones_[RGB_DIM];
	int chIdx_[RGB_DIM];
	enum {
		iR, iG, iB
	};
	TriglavPlugInPropertyService* PropertyService_;


	//	プロパティコールバック
	void _PropertyCB(TriglavPlugInInt* result, TriglavPlugInPropertyObject propertyObject, const TriglavPlugInInt itemKey, const TriglavPlugInInt notify) {
		(*result) = kTriglavPlugInPropertyCallBackResultNoModify;

		if (PropertyService_ == NULL) return;

		if (notify == kTriglavPlugInPropertyCallBackNotifyValueChanged) {
			TriglavPlugInInt newValue;
			PropertyService_->getIntegerValueProc(&newValue, propertyObject, itemKey);

			switch (itemKey) {
			default: return;

			case ID_CaptionDR:
				if (tones_[iR] != newValue)
					tones_[iR] = newValue;
				break;
			case ID_CaptionDG:
				if (tones_[iG] != newValue)
					tones_[iG] = newValue;
				break;
			case ID_CaptionDB:
				if (tones_[iB] != newValue)
					tones_[iB] = newValue;
				break;
			}
			*result = kTriglavPlugInPropertyCallBackResultModify;
		}
	}


public:
	//	プロパティコールバック
	static void TRIGLAV_PLUGIN_CALLBACK PropertyCB(TriglavPlugInInt* result, TriglavPlugInPropertyObject propertyObject, const TriglavPlugInInt itemKey, const TriglavPlugInInt notify, TriglavPlugInPtr data) {
		static_cast<MyFilter*>(data)->_PropertyCB(result, propertyObject, itemKey, notify);
	}

	// メニュー、ダイアログの表示設定
	TriglavPlugInInt FilterInitialize(TriglavPlugInServer* pluginServer) {
		auto pRecordSuite = &pluginServer->recordSuite;
		auto hostObject = pluginServer->hostObject;
		auto pStringService = pluginServer->serviceSuite.stringService;
		auto PropertyService = pluginServer->serviceSuite.propertyService;

		if (TriglavPlugInGetFilterInitializeRecord(pRecordSuite) == NULL || pStringService == NULL || PropertyService == NULL)
			return kTriglavPlugInCallResultFailed;

		TriglavPlugInStringObject	filterCategoryName = NULL;
		TriglavPlugInStringObject	filterName = NULL;
		// リソースで定義したIDから文字列を取得 (like MAKEINTRESOURCE)
		// TriglavPlugInStringObjectは内容に直接アクセスできないのでこの方法しかない
		pStringService->createWithStringIDProc(&filterCategoryName, ID_FilterCategoryName, pluginServer->hostObject);
		pStringService->createWithStringIDProc(&filterName, ID_FilterName, pluginServer->hostObject);

		// 初期化レコードにカテゴリ名, フィルタ名を設定
		// カテゴリ名のアクセスキーに [c]
		TriglavPlugInFilterInitializeSetFilterCategoryName(pRecordSuite, hostObject, filterCategoryName, 'c');
		// フィルタ名のアクセスキーに [h]
		TriglavPlugInFilterInitializeSetFilterName(pRecordSuite, hostObject, filterName, 't');
		// 開放
		pStringService->releaseProc(filterCategoryName);
		pStringService->releaseProc(filterName);

		//	プレビュー checkbox を配置
		TriglavPlugInFilterInitializeSetCanPreview(pRecordSuite, hostObject, true);

		// カラーのラスターレイヤーのみを対象とする
		// それ以外のレイヤーは実行できないようにする
		std::vector<TriglavPlugInInt> target = { kTriglavPlugInFilterTargetKindRasterLayerRGBAlpha };
		TriglavPlugInFilterInitializeSetTargetKinds(pRecordSuite, hostObject, &target[0], target.size());


		//	プロパティの作成
		TriglavPlugInPropertyObject	propertyObject;
		PropertyService->createProc(&propertyObject);

		// dx (x)
		CreateSlider(pluginServer, propertyObject, ID_CaptionDR, ID_CaptionDR, MIN_TONES, MAX_TONES, 0, tones_[iR], 'r');
		CreateSlider(pluginServer, propertyObject, ID_CaptionDG, ID_CaptionDG, MIN_TONES, MAX_TONES, 0, tones_[iG], 'g');
		CreateSlider(pluginServer, propertyObject, ID_CaptionDB, ID_CaptionDB, MIN_TONES, MAX_TONES, 0, tones_[iB], 'b');

		//	プロパティの設定
		TriglavPlugInFilterInitializeSetProperty(pRecordSuite, hostObject, propertyObject);
		TriglavPlugInFilterInitializeSetPropertyCallBack(pRecordSuite, hostObject, PropertyCB, this);

		//	プロパティの破棄
		PropertyService->releaseProc(propertyObject);

		return kTriglavPlugInCallResultSuccess;
	}


	//	フィルタの実行
	TriglavPlugInInt FilterRun(TriglavPlugInServer* pluginServer) {
		LOG("%s: %s", APP_NAME, __func__);

		TriglavPlugInInt result = kTriglavPlugInCallResultFailed;

		auto pRecordSuite = &pluginServer->recordSuite;
		auto pOffscreenService = pluginServer->serviceSuite.offscreenService;
		auto pPropertyService = pluginServer->serviceSuite.propertyService;
		auto pBitmapService = pluginServer->serviceSuite.bitmapService;

		if (TriglavPlugInGetFilterRunRecord(pRecordSuite) == NULL || pOffscreenService == NULL || pPropertyService == NULL)
			return result;

		const auto hostObject = pluginServer->hostObject;

		TriglavPlugInPropertyObject		propertyObject;
		TriglavPlugInOffscreenObject	sourceOffscreenObject;
		TriglavPlugInOffscreenObject	destinationOffscreenObject;
		TriglavPlugInRect				selectAreaRect={};
		TriglavPlugInOffscreenObject	selectAreaOffscreenObject;
		TriglavPlugInFilterRunGetProperty(pRecordSuite, &propertyObject, hostObject);
		TriglavPlugInFilterRunGetSourceOffscreen(pRecordSuite, &sourceOffscreenObject, hostObject);
		TriglavPlugInFilterRunGetDestinationOffscreen(pRecordSuite, &destinationOffscreenObject, hostObject);
		TriglavPlugInFilterRunGetSelectAreaRect(pRecordSuite, &selectAreaRect, hostObject);
		TriglavPlugInFilterRunGetSelectAreaOffscreen(pRecordSuite, &selectAreaOffscreenObject, hostObject);

		// フィルタの処理ごとにプロパティサービスを更新しないとクラッシュする。
		this->PropertyService_ = pPropertyService;
		// スライダーは0にリセットされてしまうが、この値はそのまま。明示的に0にしないとそのぶんずれになってしまう
		//this->displacementX_ = 0;
		// 0戻しは面倒なので、スライダーの位置を強制的に移動することにした
		pPropertyService->setIntegerValueProc(propertyObject, ID_CaptionDR, tones_[iR]);
		pPropertyService->setIntegerValueProc(propertyObject, ID_CaptionDG, tones_[iG]);
		pPropertyService->setIntegerValueProc(propertyObject, ID_CaptionDB, tones_[iB]);

		//auto Blend8 = [](const int dst, const int src, const int mask) -> int {
		//	return ((dst - src) * mask / 255) + src;
		//};

		// Get image RECT
		TriglavPlugInInt width, height;//, top, left;
		if (selectAreaOffscreenObject != NULL) {
			//top = selectAreaRect.top;
			//left = selectAreaRect.left;
			width = abs(selectAreaRect.left - selectAreaRect.right);
			height = abs(selectAreaRect.top - selectAreaRect.bottom);
		}
		else {
			//top = 0;
			//left = 0;
			pOffscreenService->getWidthProc(&width, destinationOffscreenObject);
			pOffscreenService->getHeightProc(&height, destinationOffscreenObject);
		}

		// 元画像Bitmap を取得
		TriglavPlugInBitmapObject sourceBitmapObject = NULL;
		pBitmapService->createProc(&sourceBitmapObject, width, height, RGB_DIM, kTriglavPlugInBitmapScanlineHorizontalLeftTop);
		// 画像の作業領域bitmap。1ライン目は編集元のラインを入れる、2ライン目は処理されたラインを格納
		TriglavPlugInBitmapObject srcBmpLine = NULL;
		pBitmapService->createProc(&srcBmpLine, width, 2, RGB_DIM, kTriglavPlugInBitmapScanlineHorizontalLeftTop);

		// 描画データをbitmapにコピー
		TriglavPlugInPoint bitmapSourcePoint{ 0, 0 };
		//TriglavPlugInPoint offscreenSourcePoint{ left, top };
		TriglavPlugInPoint offscreenSourcePoint{ selectAreaRect.left, selectAreaRect.top };
		pOffscreenService->getBitmapProc(sourceBitmapObject, &bitmapSourcePoint, sourceOffscreenObject, &offscreenSourcePoint, width, height, kTriglavPlugInOffscreenCopyModeImage);
		
		// Get (R,G,B) indices
		TriglavPlugInInt chIdx[RGB_DIM] {};
		pOffscreenService->getRGBChannelIndexProc(&chIdx[0], &chIdx[1], &chIdx[2], sourceOffscreenObject);

		// 進捗の最大値設定 (max = height)
		TriglavPlugInFilterRunSetProgressTotal(pRecordSuite, hostObject, height);

		/*
		TriglavPlugInInt rowBytes, pixelBytes, depth;
		pBitmapService->getRowBytesProc(&rowBytes, sourceBitmapObject);
		pBitmapService->getPixelBytesProc(&pixelBytes, sourceBitmapObject);
		pBitmapService->getDepthProc(&depth, sourceBitmapObject);
		LOG("rowBytes, pixelBytes, depth=%d, %d, %d", rowBytes, pixelBytes, depth);
		*/

		bool restart = true;
		//long steps = 0;

		BYTE	colors[RGB_DIM][0x100];

		for (long y = 0; true; ) {
			TriglavPlugInInt processResult;

			if (restart) { // init
				restart = false;
				y = 0; // Y座標を戻す
				//steps = this->displacementX_;	// ダイアログ設定値で更新
				//LOG("steps=%d", steps);

				TriglavPlugInFilterRunProcess(pRecordSuite, &processResult, hostObject, kTriglavPlugInFilterRunProcessStateStart);
				if (processResult == kTriglavPlugInFilterRunProcessResultExit) break;

				// No needed to process
				//if (steps < 2) y = height;

				// create tone table
				// becareful 0-DIV!!!
				rep(c, RGB_DIM)
					rep(i, tones_[c])	colors[c][i] = 255 * i / (tones_[c] - 1);
			}

			TriglavPlugInPoint bmpLinePtSrc{ 0, 0 };
			TriglavPlugInPoint bmpLinePtDst{ 0, 1 };
			//TriglavPlugInPoint ofsLinePt{ left, top + y };
			TriglavPlugInPoint ofsLinePt{ selectAreaRect.left, selectAreaRect.top + y };

			if (y < height) {
				// ラインの処理 一度に全体のbitmapを取得しながらだと、プレビュがとても重くなってしまう
				pOffscreenService->getBitmapProc(srcBmpLine, &bmpLinePtSrc, sourceOffscreenObject, &ofsLinePt, width, 1, kTriglavPlugInOffscreenCopyModeImage);

				if (selectAreaOffscreenObject == NULL) { // 選択範囲なし、画像全体を更新
					rep(x, width) {
						BYTE* address;
						TriglavPlugInPoint point = { x, 0 };
						pBitmapService->getAddressProc((TriglavPlugInPtr*)&address, srcBmpLine, &point);
						auto src = address;

						point = { x, 1 };
						pBitmapService->getAddressProc((TriglavPlugInPtr*)&address, srcBmpLine, &point);
						auto dst = address;

						for(auto c : chIdx)
							dst[c] = colors[c][(src[c] * tones_[c]) >> 8];
						
					} 
				} else { // 選択範囲あり
					// 2ライン目にも1ライン目と同じ画素をコピー。なぜなら、選択範囲があると更新しない画素があるので、そこに以前の画素がないと、選択範囲外が黒くなる
					pOffscreenService->getBitmapProc(srcBmpLine, &bmpLinePtDst, sourceOffscreenObject, &ofsLinePt, width, 1, kTriglavPlugInOffscreenCopyModeImage);
					//LOG("w/ selection")
					TriglavPlugInRect	tempRect;

					rep(x, width) {
						BYTE* pSelectAddressX, *pDstAlphaAddressX;
						//TriglavPlugInPoint	pos={x,y};
						TriglavPlugInPoint	pos={x + selectAreaRect.left, y+ selectAreaRect.top};

						TriglavPlugInInt dstAlphaRowBytes, dstAlphaPixelBytes;
						pOffscreenService->getBlockAlphaProc((TriglavPlugInPtr*)&pDstAlphaAddressX, &dstAlphaRowBytes, &dstAlphaPixelBytes, &tempRect, destinationOffscreenObject, &pos);

						// posは選択範囲内か?　外ならselectAddress=NULL
						TriglavPlugInInt selectRowBytes, selectPixelBytes;
						pOffscreenService->getBlockSelectAreaProc((TriglavPlugInPtr*)&pSelectAddressX, &selectRowBytes, &selectPixelBytes, &tempRect, selectAreaOffscreenObject, &pos);
						//auto pSelectAddressX = selectAddress;
						//auto pDstAlphaAddressX = dstAlphaAddress;
						
						if (pSelectAddressX != NULL) {
							//if (*pDstAlphaAddressX > 0) {
							if (*pSelectAddressX != 0) { // 薄い選択範囲が存在します、フィルタの効果も薄くなるように、別の計算
								BYTE* address;
								TriglavPlugInPoint point = { x, 0 };
								pBitmapService->getAddressProc((TriglavPlugInPtr*)&address, srcBmpLine, &point);
								auto src = address;

								point = { x, 1 };
								pBitmapService->getAddressProc((TriglavPlugInPtr*)&address, srcBmpLine, &point);
								auto dst = address;

								for (auto c : chIdx)
									dst[c] = colors[c][(src[c] * tones_[c]) >> 8];
								//PIHSVFilter::SetHSV8Mask(pDstImageAddressX[r], pDstImageAddressX[g], pDstImageAddressX[b], *pSelectAddressX, hue, saturation, value);
							}
							// *pSelectAddressX == 0 の時は選択範囲外です。そのため、処理none
						//}
						}
						//pSelectAddressX += selectPixelBytes;
					}
				}

				// copy line to the specific area of bitmap
				pOffscreenService->setBitmapProc(destinationOffscreenObject, &ofsLinePt, srcBmpLine, &bmpLinePtDst, width, 1, kTriglavPlugInOffscreenCopyModeImage);
				y++; // goto next row
			}

			if (selectAreaOffscreenObject == NULL) {
				const TriglavPlugInRect rect{ 0, 0, width, height };
				TriglavPlugInFilterRunUpdateDestinationOffscreenRect(pRecordSuite, hostObject, &rect);
			}
			else {
				TriglavPlugInFilterRunUpdateDestinationOffscreenRect(pRecordSuite, hostObject, &selectAreaRect);
			}

			if (y < height) { // in progresws
				TriglavPlugInFilterRunProcess(pRecordSuite, &processResult, hostObject, kTriglavPlugInFilterRunProcessStateContinue);
			}
			else { // done
				TriglavPlugInFilterRunSetProgressDone(pRecordSuite, hostObject, height); // 100%
				TriglavPlugInFilterRunProcess(pRecordSuite, &processResult, hostObject, kTriglavPlugInFilterRunProcessStateEnd);
			}

			// 再実行
			if (processResult == kTriglavPlugInFilterRunProcessResultRestart)
				restart = true;
			// 終了する必要がこの場合、処理のメインループから抜ける
			else if (processResult == kTriglavPlugInFilterRunProcessResultExit)
				break;
		}

		result = kTriglavPlugInCallResultSuccess;
		LOG("SUCCESS");

		return result;
	}

	//MyFilter(TriglavPlugInPropertyService *propService) : displacementX_(0), PropertyService_(propService) {}
	MyFilter() : PropertyService_(NULL) {
		for(auto &it :tones_) it = MIN_TONES;
	}
};


TriglavPlugInInt ModuleInitialize(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	TriglavPlugInInt ret = kTriglavPlugInCallResultFailed;

	// 初期化レコード
	auto pModuleInitializeRecord = pluginServer->recordSuite.moduleInitializeRecord;
	// 文字列サービス (文字列を扱う関数を提供)
	auto pStringService = pluginServer->serviceSuite.stringService;

	if (pModuleInitializeRecord != NULL && pStringService != NULL) {
		// ホスト
		auto hostObject = pluginServer->hostObject;
		// ホストのバージョンを取得
		TriglavPlugInInt hostVersion;
		pModuleInitializeRecord->getHostVersionProc(&hostVersion, hostObject);

		// ホストのバージョンが、このプラグインが必要としているバージョン以上なら登録処理
		if (hostVersion >= kTriglavPlugInNeedHostVersion) {
			// モジュール初期化レコードに moduleID を設定。
			TriglavPlugInStringObject	moduleID = NULL;

			pStringService->createWithAsciiStringProc(&moduleID, MODULE_ID.c_str(), MODULE_ID.size());
			pModuleInitializeRecord->setModuleIDProc(hostObject, moduleID);
			// 初期化レコードにプラグインの種類を設定 (フィルタのプラグイン = kTriglavPlugInModuleSwitchKindFilter)
			pModuleInitializeRecord->setModuleKindProc(hostObject, kTriglavPlugInModuleSwitchKindFilter);
			pStringService->releaseProc(moduleID);

			// Create filter instance
			//*data = new MyFilter(pluginServer->serviceSuite.propertyService);
			*data = new MyFilter;
			if(*data == NULL) return ret;

			// 登録成功
			ret = kTriglavPlugInCallResultSuccess;
			LOG("%s: SUCCESS: %s", APP_NAME, MODULE_ID.c_str())
		}
	}

	return ret;
}

TriglavPlugInInt ModuleTerminate(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	delete static_cast<MyFilter*>(*data);
	*data = NULL;
	return kTriglavPlugInCallResultSuccess;
}

TriglavPlugInInt FilterTerminate(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	return kTriglavPlugInCallResultSuccess;
}


void TRIGLAV_PLUGIN_API TriglavPluginCall(TriglavPlugInInt* result, TriglavPlugInPtr* data, TriglavPlugInInt selector, TriglavPlugInServer* pluginServer, TriglavPlugInPtr reserved) {
	*result = kTriglavPlugInCallResultFailed;

	if (pluginServer == NULL) return;

	switch (selector) {
	case kTriglavPlugInSelectorModuleInitialize:
		// プラグインモジュールの初期化
		*result = ModuleInitialize(data, pluginServer);
		break;

	case kTriglavPlugInSelectorModuleTerminate:
		//	プラグインの終了処理
		*result = ModuleTerminate(data, pluginServer);
		break;

	case kTriglavPlugInSelectorFilterTerminate:
		//	フィルタの終了処理
		*result = FilterTerminate(data, pluginServer);
		break;

	case kTriglavPlugInSelectorFilterInitialize:
		// フィルタの初期化
		*result = static_cast<MyFilter*>(*data)->FilterInitialize(pluginServer);
		break;

	case kTriglavPlugInSelectorFilterRun:
		// フィルタの計算処理 (本体)
		*result = static_cast<MyFilter*>(*data)->FilterRun(pluginServer);
		break;
	}
}
