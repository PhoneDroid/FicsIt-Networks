#include "FicsItKernel.h"

#include "Processor/Lua/LuaProcessor.h"

#include "SML/util/Logging.h"

namespace FicsItKernel {
	KernelCrash::KernelCrash(std::string what) : std::exception(what.c_str()) {}

	KernelCrash::~KernelCrash() {}

	KernelSystem::KernelSystem(UWorld* world) : world(world), listener(new KernelListener(this)) {}

	KernelSystem::~KernelSystem() {}

	void KernelSystem::tick(float deltaSeconds) {
		if (getState() == RESET) if (!start(true)) return;
		if (getState() == RUNNING) {
			devDevice->tickListeners();
			processor->tick(deltaSeconds);
		}
	}

	FicsItFS::Root* KernelSystem::getFileSystem() {
		if (getState() == RUNNING) return &filesystem;
		return nullptr;
	}

	void KernelSystem::setCapacity(std::int64_t capacity) {
		memoryCapacity = capacity;
		if (getState() == RUNNING) start(true);
	}

	std::int64_t KernelSystem::getCapacity() const {
		return memoryCapacity;
	}

	void KernelSystem::setProcessor(Processor* processor) {
		this->processor = std::unique_ptr<Processor>(processor);
		if (getProcessor()) processor->setKernel(this);
	}

	Processor* KernelSystem::getProcessor() const {
		return processor.get();
	}

	KernelCrash KernelSystem::getCrash() const {
		return kernelCrash;
	}

	KernelState KernelSystem::getState() const {
		return state;
	}

	void KernelSystem::addDrive(AFINFileSystemState* drive) {
		// check if drive is added & return if found
		if (drives.find(drive) != drives.end()) return;

		// create & assign device from drive
		drives[drive] = drive->createDevice();

		// add drive to devDevice
		if (devDevice.isValid()) {
			if (!devDevice->addDevice(drives[drive], TCHAR_TO_UTF8(*drive->ID.ToString()))) drives.erase(drive);
		}
	}

	void KernelSystem::removeDrive(AFINFileSystemState* drive) {
		// try to find location of drive
		auto s = drives.find(drive);
		if (s == drives.end()) return;

		// remove drive from devDevice
		if (devDevice.isValid()) {
			if (!devDevice->removeDevice(s->second)) return;
		}

		// remove drive from filesystem
		drives.erase(s);
	}

	bool KernelSystem::initFileSystem(FileSystem::Path path) {
		if (getState() == RUNNING) {
			return filesystem.mount(devDevice, path);
		} else return false;
	}

	bool KernelSystem::start(bool reset) {
		// check state and stop if needed
		if (getState() == RUNNING) {
			if (reset) {
				if (!stop()) return false;
			} else return false;
		} else if (getProcessor() == nullptr) {
			crash(KernelCrash("No processor set"));
			return false;
		}

		// set state and clear kernel crash
		state = RUNNING;
		kernelCrash = KernelCrash("");

		// reset whole system (filesystem, memory, processor, signal stuff)
		filesystem = FicsItFS::Root();
		filesystem.addListener(listener);
		memoryUsage = 0;
		processor->reset();

		// create & init devDevice
		devDevice = new FicsItFS::DevDevice();
		for (auto& drive : drives) {
			devDevice->addDevice(drive.second, TCHAR_TO_UTF8(*drive.first->ID.ToString()));
		}

		// finish start
		return true;
	}

	bool KernelSystem::reset() {
		if (stop()) {
			state = RESET;
			return true;
		}
		return false;
	}

	bool KernelSystem::stop() {
		// check state
		if (getState() != RUNNING) return false;

		// set state
		state = SHUTOFF;

		// clear filesystem
		filesystem = FicsItFS::Root();

		// finish stop
		return true;
	}

	void KernelSystem::crash(KernelCrash crash) {
		// check state
		if (getState() != RUNNING) return;

		// set state & crash
		state = CRASHED;
		kernelCrash = crash;
		SML::Logging::error("LUA Crash: ", kernelCrash.what());
	}

	void KernelSystem::recalculateResources(Recalc components) {
		memoryUsage = processor->getMemoryUsage(components & PROCESSOR);
		memoryUsage += filesystem.getMemoryUsage(components & FILESYSTEM);

		if (memoryUsage > memoryCapacity) crash({"out of memory"});
		FileSystem::SRef<FicsItFS::DevDevice> dev = filesystem.getDevDevice();
		if (dev) dev->updateCapacity(memoryCapacity - memoryUsage);
	}

	void KernelSystem::setNetwork(Network::NetworkController* controller) {
		network.reset(controller);
	}

	Network::NetworkController* KernelSystem::getNetwork() {
		return network.get();
	}

	std::int64_t KernelSystem::getMemoryUsage() {
		return memoryUsage;
	}

	KernelListener::KernelListener(KernelSystem* parent) : parent(parent) {}

	void KernelListener::onMounted(FileSystem::Path path, FileSystem::SRef<FileSystem::Device> device) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 4, path.str());
	}

	void KernelListener::onUnmounted(FileSystem::Path path, FileSystem::SRef<FileSystem::Device> device) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 5, path.str());
	}

	void KernelListener::onNodeAdded(FileSystem::Path path, FileSystem::NodeType type) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 0, path.str(), (int)type);
	}

	void KernelListener::onNodeRemoved(FileSystem::Path path, FileSystem::NodeType type) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 1, path.str(), (int)type);
	}

	void KernelListener::onNodeChanged(FileSystem::Path path, FileSystem::NodeType type) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 2, path.str(), (int)type);
	}

	void KernelListener::onNodeRenamed(FileSystem::Path newPath, FileSystem::Path oldPath, FileSystem::NodeType type) {
		parent->getNetwork()->pushSignalKernel("FileSystemUpdate", 3, newPath.str(), oldPath.str(), (int)type);
	}
}