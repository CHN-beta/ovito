////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/ExecutionContext.h>

namespace Ovito {

/**
 * \brief Data structure passed to constructors of RefTarget derived classes.
 */
class OVITO_CORE_EXPORT ObjectCreationParams final
{
public:
	enum InitializationFlag {
		NoFlags = 0,
        DontCreateSubObjects = (1 << 0),//< Used when an object is being cloned or deserialized from a file stream.
        LoadUserDefaults = (1 << 1),    //< Load user-defined standard values from the application settings store.
	    WithoutVisElement = (1 << 2),	//< Do not attach a standard visual element when creating a new data object.
	};
	Q_DECLARE_FLAGS(InitializationFlags, InitializationFlag);

	constexpr explicit ObjectCreationParams(DataSet* dataset) noexcept : _dataset(dataset) {}
	constexpr explicit ObjectCreationParams(DataSet* dataset, InitializationFlags flags) noexcept : _dataset(dataset), _flags(flags) {}

	constexpr DataSet* dataset() const { return _dataset; }
	constexpr InitializationFlags flags() const { return _flags; }
    constexpr bool dontCreateSubObjects() const { return _flags.testFlag(DontCreateSubObjects); }
    constexpr bool createSubObjects() const { return !dontCreateSubObjects(); }
    constexpr bool loadUserDefaults() const { return _flags.testFlag(LoadUserDefaults); }
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    constexpr bool createVisElement() const { return !_flags.testAnyFlags(InitializationFlags(DontCreateSubObjects | WithoutVisElement)); }
#else
    constexpr bool createVisElement() const { return !_flags.testFlag(DontCreateSubObjects) && !_flags.testFlag(WithoutVisElement); }
#endif

private:
	DataSet* _dataset;
	InitializationFlags _flags{NoFlags};
};

/**
 * \brief A smart-pointer to an OvitoObject.
 *
 * This smart-pointer class takes care of incrementing and decrementing
 * the reference counter of the object it is pointing to. As soon as no
 * OORef pointer to an object instance is left, the referenced object is automatically
 * destroyed.
 */
template<class T>
class OORef
{
private:

	using this_type = OORef;

    T* px = nullptr;

	template<class U> friend class OORef;
    template<class T2, class U> OORef<T2> friend static_pointer_cast(OORef<U>&& p) noexcept;
    template<class T2, class U> OORef<T2> friend dynamic_pointer_cast(OORef<U>&& p) noexcept;
    template<class T2, class U> OORef<T2> friend const_pointer_cast(OORef<U>&& p) noexcept;

public:

    using element_type = T;

    /// Default constructor.
    OORef() noexcept = default;

    /// Null constructor.
    OORef(std::nullptr_t) noexcept : px(nullptr) {}

    /// Initialization constructor.
    OORef(const T* p) noexcept : px(const_cast<T*>(p)) {
    	if(px) px->incrementReferenceCount();
    }

    /// Copy constructor.
    OORef(const OORef& rhs) noexcept : px(rhs.get()) {
    	if(px) px->incrementReferenceCount();
    }

    /// Copy and conversion constructor.
    template<class U>
    OORef(const OORef<U>& rhs) noexcept : px(rhs.get()) {
    	if(px) px->incrementReferenceCount();
    }

    /// Move constructor.
    OORef(OORef&& rhs) noexcept : px(rhs.px) {
    	rhs.px = nullptr;
    }

    /// Move and conversion constructor.
    template<class U>
    OORef(OORef<U>&& rhs) noexcept : px(rhs.px) {
    	rhs.px = nullptr;
    }

    /// Destructor.
    ~OORef() {
    	if(px) px->decrementReferenceCount();
    }

    template<class U>
    OORef& operator=(const OORef<U>& rhs) {
    	this_type(rhs).swap(*this);
    	return *this;
    }

    OORef& operator=(const OORef& rhs) {
    	this_type(rhs).swap(*this);
    	return *this;
    }

    OORef& operator=(OORef&& rhs) noexcept {
    	this_type(std::move(rhs)).swap(*this);
    	return *this;
    }

    template<class U>
    OORef& operator=(OORef<U>&& rhs) noexcept {
    	this_type(std::move(rhs)).swap(*this);
    	return *this;
    }

    OORef& operator=(const T* rhs) {
    	this_type(rhs).swap(*this);
    	return *this;
    }

    void reset() {
    	this_type().swap(*this);
    }

    void reset(const T* rhs) {
    	this_type(rhs).swap(*this);
    }

    inline T* get() const noexcept {
    	return px;
    }

    inline operator T*() const noexcept {
    	return px;
    }

    inline T& operator*() const noexcept {
    	OVITO_ASSERT(px != nullptr);
    	return *px;
    }

    inline T* operator->() const noexcept {
    	OVITO_ASSERT(px != nullptr);
    	return px;
    }

    inline void swap(OORef& rhs) noexcept {
    	std::swap(px,rhs.px);
    }

    /// Factory method that instantiates and initializes a new object (any RefTarget derived class).
    template<typename... Args>
	static this_type create(ObjectCreationParams params, Args&&... args) {
        using OType = std::remove_const_t<T>;
        static_assert(std::is_base_of_v<Ovito::RefTarget, OType>, "Object class must be a RefTarget derived class");
        OVITO_ASSERT((params.dataset() || std::is_base_of_v<Ovito::DataSet, OType>));
		OORef<OType> obj(new OType(params, std::forward<Args>(args)...));
        if(params.loadUserDefaults())
            obj->initializeParametersToUserDefaults();
        return obj;
	}

    /// Factory method that instantiates a new object.
    template<typename... Args>
	static this_type create(DataSet* dataset, ObjectCreationParams::InitializationFlag extraFlag, Args&&... args) {
        return create(dataset, ObjectCreationParams::InitializationFlags(extraFlag), std::forward<Args>(args)...);
    }

    /// Factory method that instantiates a new object.
    template<typename... Args>
	static this_type create(DataSet* dataset, ObjectCreationParams::InitializationFlags extraFlags, Args&&... args) {
        return create(ObjectCreationParams(dataset,
            extraFlags | (ExecutionContext::isInteractive() ? ObjectCreationParams::LoadUserDefaults : ObjectCreationParams::NoFlags)), 
            std::forward<Args>(args)...);
    }

    /// Factory method that instantiates a new object.
    template<typename... Args>
	static this_type create(DataSet* dataset, Args&&... args) {
        using OType = std::remove_const_t<T>;
        if constexpr(std::is_base_of_v<Ovito::RefTarget, OType>) {
            // All RefTarget derived classes expect a ObjectCreationParams structure.
            return create(ObjectCreationParams(dataset, 
                ExecutionContext::isInteractive() 
                    ? ObjectCreationParams::LoadUserDefaults 
                    : ObjectCreationParams::NoFlags), std::forward<Args>(args)...);
        }
        else {
            // Objects not derived from RefTarget do not expect a ObjectCreationParams.
    		return new OType(std::forward<Args>(args)...);
        }
	}
};

template<class T, class U> inline bool operator==(const OORef<T>& a, const OORef<U>& b) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(const OORef<T>& a, const OORef<U>& b) noexcept
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator==(const OORef<T>& a, U* b) noexcept
{
    return a.get() == b;
}

template<class T, class U> inline bool operator!=(const OORef<T>& a, U* b) noexcept
{
    return a.get() != b;
}

template<class T, class U> inline bool operator==(T* a, const OORef<U>& b) noexcept
{
    return a == b.get();
}

template<class T, class U> inline bool operator!=(T* a, const OORef<U>& b) noexcept
{
    return a != b.get();
}

template<class T> inline bool operator==(const OORef<T>& p, std::nullptr_t) noexcept
{
    return p.get() == nullptr;
}

template<class T> inline bool operator==(std::nullptr_t, const OORef<T>& p) noexcept
{
    return p.get() == nullptr;
}

template<class T> inline bool operator!=(const OORef<T>& p, std::nullptr_t) noexcept
{
    return p.get() != nullptr;
}

template<class T> inline bool operator!=(std::nullptr_t, const OORef<T>& p) noexcept
{
    return p.get() != nullptr;
}

template<class T> inline bool operator<(const OORef<T>& a, const OORef<T>& b) noexcept
{
    return std::less<T*>()(a.get(), b.get());
}

template<class T> void swap(OORef<T>& lhs, OORef<T>& rhs) noexcept
{
	lhs.swap(rhs);
}

template<class T> T* get_pointer(const OORef<T>& p) noexcept
{
    return p.get();
}

template<class T, class U> OORef<T> static_pointer_cast(const OORef<U>& p) noexcept
{
    return static_cast<T*>(p.get());
}

template<class T, class U> OORef<T> static_pointer_cast(OORef<U>&& p) noexcept
{
    OORef<T> result;
    result.px = static_cast<T*>(p.get());
    p.px = nullptr;
    return result;
}

template<class T, class U> OORef<T> const_pointer_cast(const OORef<U>& p) noexcept
{
    return const_cast<T*>(p.get());
}

template<class T, class U> OORef<T> const_pointer_cast(OORef<U>&& p) noexcept
{
    OORef<T> result;
    result.px = const_cast<T*>(p.get());
    p.px = nullptr;
    return result;
}

template<class T, class U> OORef<T> dynamic_pointer_cast(const OORef<U>& p) noexcept
{
    return qobject_cast<T*>(p.get());
}

template<class T, class U> OORef<T> dynamic_pointer_cast(OORef<U>&& p) noexcept
{
    OORef<T> result;
    result.px = qobject_cast<T*>(p.get());
    if(result.px)
        p.px = nullptr;
    return result;
}

template<class T> QDebug operator<<(QDebug debug, const OORef<T>& p)
{
	return debug << p.get();
}

}	// End of namespace
