#pragma once

#include "V8DataModel/PartInstance.h"
#include "Util/PV.h"
#include "Util/Memory.h"
#include "rbx/Debug.h"
#include "G3D/Array.h"
#include "CompactCFrame.h"
#include "RakNetTime.h"

namespace RBX {

	class Primitive;

	class AssemblyItem : public boost::noncopyable
	{
	public:
		shared_ptr<PartInstance> rootPart;
		RBX::PV pv;
		G3D::Array<CompactCFrame> motorAngles;

		void reset() {
			rootPart.reset();
			motorAngles.fastClear();
		}

		AssemblyItem() {
			reset();
		}
	};


	class MechanismItem : boost::noncopyable
	{
	private:
		// TODO: Replace with intrusive list of AssemblyItem
		G3D::Array<AssemblyItem*> buffer;

		size_t currentElements;

	public:
		RakNet::Time networkTime;
		unsigned char networkHumanoidState;
		bool hasVelocity;

		void reset(size_t numElements = 0u);

		MechanismItem() {
			reset();
		}

		~MechanismItem();

		AssemblyItem& appendAssembly();

		size_t numAssemblies() const { return currentElements; }

		AssemblyItem& getAssemblyItem(size_t i) const {
			RBXASSERT(i < currentElements);
			return *buffer[i];
		}

		static bool consistent(const MechanismItem* before, const MechanismItem* after);

		static void lerp(const MechanismItem* before, const MechanismItem* after, MechanismItem* out, float lerpAlpha);
	};
}
