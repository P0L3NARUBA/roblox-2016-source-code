#pragma once

#include "ImageInfo.h"

#include <rbx/Boost.hpp>

namespace RBX {
	namespace Graphics {

		class Texture;

		class TextureRef {
		public:
			enum Status {
				Status_Null,
				Status_Waiting,
				Status_Failed,
				Status_Loaded
			};

			static TextureRef future(const std::shared_ptr<Texture>& fallback);

			TextureRef()
			{
			}

			TextureRef(const std::shared_ptr<Texture>& texture, Status status = Status_Loaded);

			TextureRef clone() const;

			void updateAllRefs(const std::shared_ptr<Texture>& texture);
			void updateAllRefsToLoaded(const std::shared_ptr<Texture>& texture);
			void updateAllRefsToLoaded(const std::shared_ptr<Texture>& texture, const ImageInfo& info);
			void updateAllRefsToFailed();
			void updateAllRefsToWaiting();

			const std::shared_ptr<Texture>& getTexture() const { return data ? data->texture : emptyTexture; }

			bool isUnique() const { return data.unique(); }

			Status getStatus() const { return data ? data->status : Status_Null; }

			const ImageInfo& getInfo() const { return data ? data->info : emptyInfo; }

		private:
			struct Data {
				std::shared_ptr<Texture> texture;
				Status status;
				ImageInfo info;
			};

			std::shared_ptr<Data> data;

			static std::shared_ptr<Texture> emptyTexture;
			static ImageInfo emptyInfo;

			TextureRef(Data* data);
		};

	}
}
