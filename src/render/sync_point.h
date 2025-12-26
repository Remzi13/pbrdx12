#pragma once


namespace render {

	class Fence;

	class SyncPoint
	{
	public:
		SyncPoint() = default;
		SyncPoint(Fence* fence, size_t value)
			: fence_(fence), value_(value)
		{
		}

		void wait() const;
		bool isComplete() const;// { return fence_->isComplete(value_); }
		size_t value() const { return value_; }
		Fence* fence() const { return fence_; }
		bool isValid() const { return !!fence_; }
		operator bool() const { return isValid(); }

	private:
		Fence* fence_ = nullptr;
		size_t value_ = 0;
	};

}