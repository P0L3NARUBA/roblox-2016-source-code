#pragma once

#include "TextureRef.h"
#include "util/ContentId.h"

#include <boost/unordered_map.hpp>

#include <rbx/threadsafe.h>

namespace RBX {
	class ThreadPool;
}

namespace RBX {
	namespace Graphics {

		class VisualEngine;
		class Image;

		struct TextureManagerStats {
			uint32_t queuedCount;

			uint32_t totalBudget;

			uint32_t liveCount;
			uint32_t liveSize;

			uint32_t orphanedCount;
			uint32_t orphanedSize;
		};

		class TextureManager {
		public:
			enum Fallback {
				Fallback_None,

				Fallback_White,
				Fallback_Gray,
				Fallback_Black,
				Fallback_BlackTransparent,

				Fallback_NormalMap,
				Fallback_Reflection,

				Fallback_Count
			};

			TextureManager(VisualEngine* visualEngine);
			~TextureManager();

			void processPendingRequests();
			void cancelPendingRequests();

			void garbageCollectIncremental();
			void garbageCollectFull();

			void reloadImage(const ContentId& id, const std::string& context = "");
			TextureRef load(const ContentId& id, Fallback fallback, const std::string& context = "");

			const std::shared_ptr<Texture>& getFallbackTexture(Fallback fallback) { return fallbackTextures[fallback]; }
			bool isFallbackTexture(const shared_ptr<Texture>& tex);

			bool isQueueEmpty() const { return outstandingRequests == 0u; }

			TextureManagerStats getStatistics() const;

		private:
			struct LoadedImage {
				std::string context;
				ContentId id;

				shared_ptr<Image> image;
				ImageInfo info;

				Time::Interval loadTime;
			};

			struct TextureData {
				ContentId id;

				std::vector<TextureRef> external;
				TextureRef object;

				bool orphaned;
				TextureData* orphanedPrev;
				TextureData* orphanedNext;

				TextureData();

				TextureRef addExternalRef(const std::shared_ptr<Texture>& fallback);

				void removeUnusedExternalRefs();

				void updateAllRefsToLoaded(const std::shared_ptr<Texture>& texture, const ImageInfo& info);
				void updateAllRefsToFailed();
				void updateAllRefsToWaiting();
			};

			VisualEngine* visualEngine;

			typedef boost::unordered_map<ContentId, TextureData> Textures;
			Textures textures;

			std::shared_ptr<Texture> fallbackTextures[Fallback_Count];

			shared_ptr<rbx::safe_queue<LoadedImage> > pendingImages;
			shared_ptr<ThreadPool> loadingPool;
			boost::unordered_set<ContentId> pendingReloads;
			uint32_t outstandingRequests;

			uint32_t liveCount;
			uint32_t liveSize;

			uint32_t orphanedCount;
			uint32_t orphanedSize;

			TextureData* orphanedHead;
			TextureData* orphanedTail;

			size_t gcSizeLast;
			ContentId gcKeyNext;

			uint32_t totalSizeBudget;

			bool loadAsync(const ContentId& id, const std::string& context);

			void orphanUnusedTextures(size_t visitCount);
			void collectOrphanedTextures(uint32_t maxOrphanedSize);

			void addToOrphanedTail(TextureData* data);
			void removeFromOrphaned(TextureData* data);

			std::shared_ptr<Texture> createSingleColorTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, bool cube = false);
			std::shared_ptr<Texture> createTexture(const Image& image);

			static void loadImageHttpCallback(const weak_ptr<ThreadPool>& loadingPool, const weak_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const shared_ptr<const std::string>& content, const ContentId& id, uint32_t maxTextureSize, uint32_t flags, const std::string& context);

			static void loadImageFile(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const ContentId& loadId, uint32_t maxTextureSize, uint32_t flags, bool useRetina, const std::string& context = "");
			static void loadImageHttp(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const shared_ptr<const std::string>& content, uint32_t maxTextureSize, uint32_t flags, const std::string& context = "");
			static void loadImage(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, std::istream& stream, uint32_t maxTextureSize, uint32_t flags, uint32_t scale, const std::string& context = "");
			static void loadImageError(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const char* error, const std::string& context);
		};

	}
}
