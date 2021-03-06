/****************************************************************************
 Copyright (c) 2013-2016 Chukong Technologies Inc.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/
#include "ccHeader.h"
#include "ui/UIScale9Sprite.h"
#include "2d/CCSprite.h"
#include "2d/CCSpriteFrameCache.h"
#include "base/CCVector.h"
#include "base/CCDirector.h"
#include "base/ccUTF8.h"
#include "base/CCNinePatchImageParser.h"
#include "2d/CCDrawNode.h"
#include "renderer/Renderer.h"
#include "renderer/Program.h"

NS_CC_BEGIN
namespace ui {

    Scale9Sprite::Scale9Sprite()
        : _spriteFrameRotated(false)
        , _scale9Image(nullptr)
        , _scale9Enabled(true)
        , _insetLeft(0)
        , _insetTop(0)
        , _insetRight(0)
        , _insetBottom(0)
        ,_flippedX(false)
        ,_flippedY(false)
        ,_isPatch9(false)
        ,_brightState(State::NORMAL)
        ,_nonSliceSpriteAnchor(Vec2::ANCHOR_MIDDLE)
        ,_sliceVertices(nullptr)
        ,_sliceIndices(nullptr)
        ,_sliceSpriteDirty(false)
        ,_renderingType(RenderingType::SLICE)
        ,_insideBounds(true)
    {
        this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
#if CC_SPRITE_DEBUG_DRAW
        _debugDrawNode = DrawNode::create();
        addChild(_debugDrawNode);
#endif //CC_SPRITE_DEBUG_DRAW
    }

    Scale9Sprite::~Scale9Sprite()
    {
        this->cleanupSlicedSprites();
        CC_SAFE_RELEASE(_scale9Image);
    }

    bool Scale9Sprite::initWithFile(const Rect& capInsets, const std::string& file)
    {
        return initWithFile(file, Rect::ZERO, capInsets);
    }

    bool Scale9Sprite::initWithFile(const std::string& file)
    {
        return initWithFile(file, Rect::ZERO, Rect::ZERO);
    }

    bool Scale9Sprite::initWithSpriteFrame(SpriteFrame* spriteFrame,
                                           const Rect& capInsets)
    {
        bool ret = false;
        do {
            Texture2D* texture = spriteFrame->getTexture();
            CCASSERT(texture != nullptr, "Texture2D must be not null");
            if(texture == nullptr) break;

            Sprite *sprite = Sprite::createWithSpriteFrame(spriteFrame);
            CCASSERT(sprite != nullptr, "Sprite must be not null");
            if(sprite == nullptr) break;

            ret = this->init(sprite,
                                      spriteFrame->getRect(),
                                      spriteFrame->isRotated(),
                                      spriteFrame->getOffset(),
                                      spriteFrame->getOriginalSize(),
                                      capInsets);
        } while (false);

        return ret;
    }
    
    bool Scale9Sprite::initWithSpriteFrame(SpriteFrame* spriteFrame)
    {
        CCASSERT(spriteFrame != nullptr, "Invalid spriteFrame for sprite");
        return initWithSpriteFrame(spriteFrame, Rect::ZERO);
    }

    bool Scale9Sprite::initWithSpriteFrameName(const std::string& spriteFrameName,
                                               const Rect& capInsets)
    {
        bool ret = false;
        do {
            auto spriteFrameCache = SpriteFrameCache::getInstance();
            CCASSERT(spriteFrameCache != nullptr,
                     "SpriteFrameCache::getInstance() must be non-NULL");
            if(spriteFrameCache == nullptr) break;

            SpriteFrame *frame = spriteFrameCache->getSpriteFrameByName(spriteFrameName);
            CCASSERT(frame != nullptr, StringUtils::format("CCSpriteFrame: %s must be non-NULL ", spriteFrameName.c_str()).c_str());
            if (frame == nullptr) break;

            ret = initWithSpriteFrame(frame, capInsets);
        } while (false);

        return ret;
    }

    bool Scale9Sprite::initWithSpriteFrameName(const std::string& spriteFrameName)
    {
        return initWithSpriteFrameName(spriteFrameName, Rect::ZERO);
    }

    bool Scale9Sprite::init()
    {
        return init(nullptr, Rect::ZERO, Rect::ZERO);
    }

    bool Scale9Sprite::init(Sprite* sprite, const Rect& rect, const Rect& capInsets)
    {
        return init(sprite, rect, false, capInsets);
    }

    bool Scale9Sprite::init(Sprite* sprite,
                            const Rect& rect,
                            bool rotated,
                            const Rect& capInsets)
    {
        return init(sprite, rect, rotated, Vec2::ZERO, rect.size, capInsets);
    }

    bool Scale9Sprite::init(Sprite* sprite,
                            const Rect& rect,
                            bool rotated,
                            const Vec2 &offset,
                            const Size &originalSize,
                            const Rect& capInsets)
    {
        bool ret = true;
        if(sprite)
        {
            auto texture = sprite->getTexture();
            if (texture == nullptr) {
                return ret;
            }

            Rect actualCapInsets = capInsets;
            if (texture->isContain9PatchInfo())
            {
                auto spriteFrame = sprite->getSpriteFrame();
                auto& parsedCapInset = texture->getSpriteFrameCapInset(spriteFrame);
                if(!parsedCapInset.equals(Rect::ZERO))
                {
                    this->_isPatch9 = true;
                    if(capInsets.equals(Rect::ZERO))
                    {
                        actualCapInsets = parsedCapInset;
                    }

                }
            }

            ret = this->updateWithSprite(sprite,
                                         rect,
                                         rotated,
                                         offset,
                                         originalSize,
                                         actualCapInsets);
        }

        return ret;
    }

    bool Scale9Sprite::initWithFile(const std::string& file,
                                    const Rect& rect,
                                    const Rect& capInsets)
    {
        CCASSERT(!file.empty(), "file must not be empty string!");
        if(file.empty())
        {
            return false;
        }

        auto sprite = Sprite::create(file);
        return init(sprite, rect, capInsets);
    }

    bool Scale9Sprite::initWithFile(const std::string& file, const Rect& rect)
    {
        return initWithFile(file, rect, Rect::ZERO);
    }

    Scale9Sprite* Scale9Sprite::create()
    {
        Scale9Sprite *ret = new (std::nothrow) Scale9Sprite();
        if (ret && ret->init())
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::create(const std::string& file,
                                       const Rect& rect,
                                       const Rect& capInsets)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithFile(file, rect, capInsets) )
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }


    Scale9Sprite* Scale9Sprite::create(const std::string& file, const Rect& rect)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithFile(file, rect) )
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::create(const Rect& capInsets,
                                       const std::string& file)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithFile(capInsets, file) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }


    Scale9Sprite* Scale9Sprite::create(const std::string& file)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithFile(file) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::createWithSpriteFrame(SpriteFrame* spriteFrame,
                                                      const Rect& capInsets)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithSpriteFrame(spriteFrame, capInsets) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::createWithSpriteFrame(SpriteFrame* spriteFrame)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithSpriteFrame(spriteFrame) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::createWithSpriteFrameName(const std::string& spriteFrameName,
                                                          const Rect& capInsets)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithSpriteFrameName(spriteFrameName, capInsets) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite* Scale9Sprite::createWithSpriteFrameName(const std::string& spriteFrameName)
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->initWithSpriteFrameName(spriteFrameName) )
        {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void Scale9Sprite::cleanupSlicedSprites()
    {
        CC_SAFE_DELETE_ARRAY(_sliceIndices);
        CC_SAFE_DELETE_ARRAY(_sliceVertices);
    }


    void Scale9Sprite::setBlendFunc(const BlendFunc &blendFunc)
    {
        _blendFunc = blendFunc;
        applyBlendFunc();
    }
    const BlendFunc &Scale9Sprite::getBlendFunc() const
    {
        return _blendFunc;
    }

    void Scale9Sprite::updateBlendFunc(Texture2D *texture)
    {

        // it is possible to have an untextured sprite
        if (! texture || ! texture->hasPremultipliedAlpha())
        {
            _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
            setOpacityModifyRGB(false);
        }
        else
        {
            _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
            setOpacityModifyRGB(true);
        }
    }

    void Scale9Sprite::applyBlendFunc()
    {
        if(_scale9Image)
            _scale9Image->setBlendFunc(_blendFunc);
    }

    bool Scale9Sprite::updateWithSprite(Sprite* sprite,
                                        const Rect& rect,
                                        bool rotated,
                                        const Rect& capInsets)
    {
        return updateWithSprite(sprite, rect, rotated, Vec2::ZERO, rect.size, capInsets);
    }

    bool Scale9Sprite::updateWithSprite(Sprite* sprite,
                                        const Rect& textureRect,
                                        bool rotated,
                                        const Vec2 &offset,
                                        const Size &originalSize,
                                        const Rect& capInsets)
    {
        // Release old sprites
        this->cleanupSlicedSprites();

        updateBlendFunc(sprite?sprite->getTexture():nullptr);

        if(nullptr != sprite)
        {
            if (nullptr == sprite->getSpriteFrame())
            {
                return false;
            }

            if (nullptr == _scale9Image)
            {
                _scale9Image = sprite;
                _scale9Image->retain();
            }
            else
            {
                _scale9Image->setSpriteFrame(sprite->getSpriteFrame());
            }
        }
        else
        {
            CC_SAFE_RELEASE_NULL(_scale9Image);
        }

        if (!_scale9Image)
        {
            return false;
        }

        Rect rect(textureRect);
        Size size(originalSize);

        // If there is no given rect
        if ( rect.equals(Rect::ZERO) )
        {
            // Get the texture size as original
            Size textureSize = _scale9Image->getTexture()->getContentSize();

            rect = Rect(0, 0, textureSize.width, textureSize.height);
        }

        if( size.equals(Size::ZERO) )
        {
            size = rect.size;
        }

        // Set the given rect's size as original size
        _spriteRect = rect;
        _spriteFrameRotated = rotated;
        _originalSize = size;
        _preferredSize = size;
        _offset = offset;

        _capInsetsInternal = capInsets;

        if (_scale9Enabled)
        {
            _scale9Image->setAnchorPoint(Vec2::ZERO);
            _scale9Image->setPosition(Vec2::ZERO);
            _sliceSpriteDirty = true;
        }

        applyBlendFunc();
        if (program_) {
            _scale9Image->setProgram(program_);
        } else {
            this->setState(_brightState);
        }
        if(this->_isPatch9)
        {
            size.width = size.width - 2;
            size.height = size.height - 2;
        }
        this->setContentSize(size);

        return true;
    }

    void Scale9Sprite::configureSimpleModeRendering()
    {
        this->setInsetTop(0);
        this->setInsetBottom(0);
        this->setInsetLeft(0);
        this->setInsetRight(0);
    }

    void Scale9Sprite::createSlicedSprites()
    {
        //todo create sliced sprite
        if (_scale9Enabled)
        {
            Texture2D *tex = _scale9Image ? _scale9Image->getTexture() : nullptr;

            if (tex == nullptr)
            {
                return;
            }

            if (_renderingType == RenderingType::SIMPLE)
            {
                this->configureSimpleModeRendering();
            }

            auto capInsets = CC_RECT_POINTS_TO_PIXELS(_capInsetsInternal);
            auto textureRect = CC_RECT_POINTS_TO_PIXELS(_spriteRect);
            auto spriteRectSize = _spriteRect.size;
            auto originalSize = CC_SIZE_POINTS_TO_PIXELS(_originalSize);
            auto offset = CC_POINT_POINTS_TO_PIXELS(_offset);

            Vec4 offsets;
            offsets.x = offset.x + (originalSize.width - textureRect.size.width) / 2;
            offsets.w = offset.y + (originalSize.height - textureRect.size.height) / 2;
            offsets.z = originalSize.width - textureRect.size.width - offsets.x;
            offsets.y = originalSize.height - textureRect.size.height - offsets.w;

            //handle .9.png
            if (_isPatch9)
            {
                originalSize = Size(originalSize.width - 2, originalSize.height-2);
            }


            if(capInsets.equals(Rect::ZERO))
            {
                capInsets = Rect(originalSize.width/3, originalSize.height/3,
                                 originalSize.width/3, originalSize.height/3);
            }

            auto uv = this->calculateUV(tex, capInsets, originalSize, offsets);
            auto vertices = this->calculateVertices(capInsets, originalSize, offsets);
            auto triangles = this->calculateTriangles(uv, vertices);

            auto polyInfo = _scale9Image->getPolygonInfo();
            polyInfo.setTriangles(triangles);
            _scale9Image->setPolygonInfo(polyInfo);
        }
    }

    void Scale9Sprite::setContentSize(const Size &size)
    {
        if (_contentSize.equals(size))
        {
            return;
        }
        Node::setContentSize(size);
        _preferredSize = size;
        _sliceSpriteDirty = true;
        this->adjustNoneScale9ImagePosition();
    }

    Scale9Sprite* Scale9Sprite::resizableSpriteWithCapInsets(const Rect& capInsets) const
    {
        Scale9Sprite* ret = new (std::nothrow) Scale9Sprite();
        if ( ret && ret->init(_scale9Image,
                                      _spriteRect,
                                      _spriteFrameRotated,
                                      Vec2::ZERO,
                                      _originalSize,
                                      capInsets) )
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    Scale9Sprite::State Scale9Sprite::getState()const
    {
        return _brightState;
    }

    void Scale9Sprite::setState(cocos2d::ui::Scale9Sprite::State state)
    {
        switch (state)
        {
        case State::NORMAL:
        {
            program_ = SharedRenderer.getDefaultProgram();
        }
        break;
        case State::GRAY:
        {
            program_ = SharedRenderer.getGrayProgram();
        }
        default:
            break;
        }

        if (nullptr != _scale9Image)
        {
            _scale9Image->setProgram(program_);
        }

        _brightState = state;
    }

/** sets the opacity.
    @warning If the texture has premultiplied alpha then, the R, G and B channels will be modifed.
    Values goes from 0 to 255, where 255 means fully opaque.
*/

    void Scale9Sprite::updateCapInset()
    {
        Rect insets;
        insets = Rect(_insetLeft,
                      _insetTop,
                      _originalSize.width-_insetLeft-_insetRight,
                      _originalSize.height-_insetTop-_insetBottom);
        this->setCapInsets(insets);
    }

    void Scale9Sprite::setSpriteFrame(SpriteFrame * spriteFrame, const Rect& capInsets)
    {
        Sprite * sprite = Sprite::createWithTexture(spriteFrame->getTexture());
        this->updateWithSprite(sprite,
                               spriteFrame->getRect(),
                               spriteFrame->isRotated(),
                               spriteFrame->getOffset(),
                               spriteFrame->getOriginalSize(),
                               capInsets);

        // Reset insets
        this->_insetLeft = capInsets.origin.x;
        this->_insetTop = capInsets.origin.y;
        this->_insetRight = _originalSize.width - _insetLeft - capInsets.size.width;
        this->_insetBottom = _originalSize.height - _insetTop - capInsets.size.height;
    }

    void Scale9Sprite::setPreferredSize(const Size& preferredSize)
    {
        this->setContentSize(preferredSize);
    }

    void Scale9Sprite::setCapInsets(const Rect& capInsets)
    {
        Size contentSize = this->_contentSize;
        this->updateWithSprite(this->_scale9Image,
                               _spriteRect,
                               _spriteFrameRotated,
                               _offset,
                               _originalSize,
                               capInsets);
        this->_insetLeft = capInsets.origin.x;
        this->_insetTop = capInsets.origin.y;
        this->_insetRight = _originalSize.width - _insetLeft - capInsets.size.width;
        this->_insetBottom = _originalSize.height - _insetTop - capInsets.size.height;
        this->setContentSize(contentSize);
    }

    void Scale9Sprite::setInsetLeft(float insetLeft)
    {
        this->_insetLeft = insetLeft;
        this->updateCapInset();
    }

    void Scale9Sprite::setInsetTop(float insetTop)
    {
        this->_insetTop = insetTop;
        this->updateCapInset();
    }

    void Scale9Sprite::setInsetRight(float insetRight)
    {
        this->_insetRight = insetRight;
        this->updateCapInset();
    }

    void Scale9Sprite::setInsetBottom(float insetBottom)
    {
        this->_insetBottom = insetBottom;
        this->updateCapInset();
    }

    void Scale9Sprite::draw(IRenderer *renderer, const Mat4 &transform, uint32_t flags)
    {
        if (_scale9Image && _scale9Enabled) {
            if (_director->isCullingEnabled()) {
                // Don't do calculate the culling if the transform was not updated
                if (flags & FLAGS_TRANSFORM_DIRTY || flags & FLAGS_CULLING_DIRTY) {
                    _insideBounds = true;// renderer->checkVisibility(transform, _contentSize);
                }
            }
            else {
                _insideBounds = true;
            }

            if(_insideBounds)
            {
                auto texture = _scale9Image->getTexture();
                /*auto programState = _scale9Image->getGLProgramState();
                auto blendFunc = _scale9Image->getBlendFunc();
                auto& polyInfo = _scale9Image->getPolygonInfo();
                auto globalZOrder = _scale9Image->getGlobalZOrder();
                _trianglesCommand.init(globalZOrder,texture, programState, blendFunc, polyInfo.triangles, transform, flags);
                renderer->addCommand(&_trianglesCommand);*/

#if CC_SPRITE_DEBUG_DRAW
                _debugDrawNode->clear();
                auto count = polyInfo.triangles.indexCount/3;
                auto indices = polyInfo.triangles.indices;
                auto verts = polyInfo.triangles.verts;
                for(ssize_t i = 0; i < count; i++)
                {
                    //draw 3 lines
                    Vec3 from =verts[indices[i*3]].vertices;
                    Vec3 to = verts[indices[i*3+1]].vertices;
                    _debugDrawNode->drawLine(Vec2(from.x, from.y), Vec2(to.x,to.y), Color4F::WHITE);

                    from =verts[indices[i*3+1]].vertices;
                    to = verts[indices[i*3+2]].vertices;
                    _debugDrawNode->drawLine(Vec2(from.x, from.y), Vec2(to.x,to.y), Color4F::WHITE);

                    from =verts[indices[i*3+2]].vertices;
                    to = verts[indices[i*3]].vertices;
                    _debugDrawNode->drawLine(Vec2(from.x, from.y), Vec2(to.x,to.y), Color4F::WHITE);
                }
#endif //CC_SPRITE_DEBUG_DRAW
            }
        }
    }

    void Scale9Sprite::visit(IRenderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags)
    {
        // quick return if not visible. children won't be drawn.
        if (flags_.isOff(Node::Visible))
        {
            return;
        }
        if (_scale9Enabled && _sliceSpriteDirty) {
            this->createSlicedSprites();
            _sliceSpriteDirty = false;
        }

        uint32_t flags = processParentFlags(parentTransform, parentFlags);

        int i = 0;      // used by _children

        sortAllChildren();

        //
        // draw children and protectedChildren zOrder < 0
        //
        for( ; i < _children.size(); i++ )
        {
            auto node = _children.at(i);

            if ( node && node->getLocalZOrder() < 0 )
                node->visit(renderer, _modelViewTransform, flags);
            else
                break;
        }

        if (!_scale9Enabled && _scale9Image && _scale9Image->getLocalZOrder() < 0 )
        {
            _scale9Image->visit(renderer, _modelViewTransform, flags);
        }

        // draw self
        //
        draw(renderer, _modelViewTransform, flags);

        if (!_scale9Enabled && _scale9Image && _scale9Image->getLocalZOrder() >= 0 )
        {
            _scale9Image->visit(renderer, _modelViewTransform, flags);
        }

        for(auto it=_children.cbegin()+i; it != _children.cend(); ++it)
            (*it)->visit(renderer, _modelViewTransform, flags);

    }

    Size Scale9Sprite::getOriginalSize()const
    {
        return _originalSize;
    }


    Size Scale9Sprite::getPreferredSize() const
    {
        return _preferredSize;
    }

    Rect Scale9Sprite::getCapInsets()const
    {
        return _capInsetsInternal;
    }


    float Scale9Sprite::getInsetLeft()const
    {
        return this->_insetLeft;
    }

    float Scale9Sprite::getInsetTop()const
    {
        return this->_insetTop;
    }

    float Scale9Sprite::getInsetRight()const
    {
        return this->_insetRight;
    }

    float Scale9Sprite::getInsetBottom()const
    {
        return this->_insetBottom;
    }

    void Scale9Sprite::setScale9Enabled(bool enabled)
    {
        if (_scale9Enabled == enabled)
        {
            return;
        }
        _scale9Enabled = enabled;

        this->cleanupSlicedSprites();

        //we must invalide the transform when toggling scale9enabled
        markDirty();

        if (_scale9Enabled)
        {
            if (_scale9Image)
            {
                this->updateWithSprite(this->_scale9Image,
                                       _spriteRect,
                                       _spriteFrameRotated,
                                       Vec2::ZERO,
                                       _originalSize,
                                       _capInsetsInternal);
            }
        }
        else
        {
            if (_scale9Image)
            {
                auto quad = _scale9Image->getQuad();
                PolygonInfo polyInfo;
                polyInfo.setQuad(&quad);
                _scale9Image->setPolygonInfo(polyInfo);
            }

        }
        this->adjustNoneScale9ImagePosition();
    }

    bool Scale9Sprite::isScale9Enabled() const
    {
        return _scale9Enabled;
    }

    void Scale9Sprite::setAnchorPoint(const cocos2d::Vec2 &position)
    {
        Node::setAnchorPoint(position);
        if (!_scale9Enabled)
        {
            if (_scale9Image)
            {
                _nonSliceSpriteAnchor = position;
                _scale9Image->setAnchorPoint(position);
                this->adjustNoneScale9ImagePosition();
            }
        }
    }

    void Scale9Sprite::adjustNoneScale9ImagePosition()
    {
        if (_scale9Image)
        {
            if (!_scale9Enabled) {
                _scale9Image->setAnchorPoint(_nonSliceSpriteAnchor);
                _scale9Image->setPosition(_contentSize.width * _scale9Image->getAnchorPoint().x,
                                          _contentSize.height * _scale9Image->getAnchorPoint().y);

            }
        }
    }

    void Scale9Sprite::updateDisplayedColor(const cocos2d::Color3B &parentColor)
    {
        _displayedColor.r = _realColor.r * parentColor.r/255.0;
        _displayedColor.g = _realColor.g * parentColor.g/255.0;
        _displayedColor.b = _realColor.b * parentColor.b/255.0;
        updateColor();

        if (_scale9Image)
        {
            _scale9Image->updateDisplayedColor(_displayedColor);
        }

        if (flags_.isOn(Node::CascadeColor))
        {
            for(const auto &child : _children)
            {
                child->updateDisplayedColor(_displayedColor);
            }
        }
    }

    void Scale9Sprite::updateDisplayedOpacity(GLubyte parentOpacity)
    {
        _displayedOpacity = _realOpacity * parentOpacity/255.0;
        updateColor();

        if (_scale9Image)
        {
            _scale9Image->updateDisplayedOpacity(_displayedOpacity);
        }

        if (flags_.isOn(Node::CascadeOpacity))
        {
            for(auto child : _children)
            {
                child->updateDisplayedOpacity(_displayedOpacity);
            }
        }
    }

    void Scale9Sprite::disableCascadeColor()
    {
        for(auto child : _children)
        {
            child->updateDisplayedColor(Color3B::WHITE);
        }

        if (_scale9Image)
        {
            _scale9Image->updateDisplayedColor(Color3B::WHITE);
        }
    }

    void Scale9Sprite::disableCascadeOpacity()
    {
        _displayedOpacity = _realOpacity;

        for(auto child : _children){
            child->updateDisplayedOpacity(255);
        }
    }

    Sprite* Scale9Sprite::getSprite()const
    {
        return _scale9Image;
    }

    void Scale9Sprite::setFlippedX(bool flippedX)
    {

        float realScale = this->getScaleX();
        _flippedX = flippedX;
        this->setScaleX(realScale);
    }

    void Scale9Sprite::setFlippedY(bool flippedY)
    {
        float realScale = this->getScaleY();
        _flippedY = flippedY;
        this->setScaleY(realScale);
    }

    bool Scale9Sprite::isFlippedX()const
    {
        return _flippedX;
    }

    bool Scale9Sprite::isFlippedY()const
    {
        return _flippedY;
    }

    void Scale9Sprite::setScaleX(float scaleX)
    {
        if (_flippedX) {
            scaleX = scaleX * -1;
        }
        Node::setScaleX(scaleX);
    }

    void Scale9Sprite::setScaleY(float scaleY)
    {
        if (_flippedY) {
            scaleY = scaleY * -1;
        }
        Node::setScaleY(scaleY);
    }

    void Scale9Sprite::setScale(float scale)
    {
        this->setScaleX(scale);
        this->setScaleY(scale);
    }

    void Scale9Sprite::setScale(float scaleX, float scaleY)
    {
        this->setScaleX(scaleX);
        this->setScaleY(scaleY);
    }

    float Scale9Sprite::getScaleX()const
    {
        float originalScale = Node::getScaleX();
        if (_flippedX)
        {
            originalScale = originalScale * -1.0f;
        }
        return originalScale;
    }

    float Scale9Sprite::getScaleY()const
    {
        float originalScale = Node::getScaleY();
        if (_flippedY)
        {
            originalScale = originalScale * -1.0f;
        }
        return originalScale;
    }

    float Scale9Sprite::getScale()const
    {
        CCASSERT(this->getScaleX() == this->getScaleY(),
                 "Scale9Sprite#scale. ScaleX != ScaleY. Don't know which one to return");
        return this->getScaleX();
    }

    // (0,0)  O = capInsets.origin
    // v0----------------------
    // |        |      |      |
    // |        |      |      |
    // v1-------O------+------|
    // |        |      |      |
    // |        |      |      |
    // v2-------+------+------|
    // |        |      |      |
    // |        |      |      |
    // v3-------------------- (1,1)  (texture coordinate is flipped)
    // u0       u1     u2     u3
    std::vector<Vec2> Scale9Sprite::calculateUV(Texture2D *tex,
                                               const Rect& capInsets,
                                               const Size& originalSize,
                                               const Vec4& offsets)
    {
        auto atlasWidth = tex->getPixelsWide();
        auto atlasHeight = tex->getPixelsHigh();

        //caculate texture coordinate
        float leftWidth = 0, centerWidth = 0, rightWidth = 0;
        float topHeight = 0, centerHeight = 0, bottomHeight = 0;

        if (_spriteFrameRotated)
        {
            rightWidth = capInsets.origin.y - offsets.y;
            centerWidth = capInsets.size.height;
            leftWidth = originalSize.height - centerWidth - capInsets.origin.y - offsets.w;

            topHeight = capInsets.origin.x - offsets.x;
            centerHeight = capInsets.size.width;
            bottomHeight = originalSize.width - (capInsets.origin.x + centerHeight) - offsets.z;
        }
        else
        {
            leftWidth = capInsets.origin.x - offsets.x;
            centerWidth = capInsets.size.width;
            rightWidth = originalSize.width - (capInsets.origin.x + centerWidth) - offsets.z;

            topHeight = capInsets.origin.y - offsets.y;
            centerHeight = capInsets.size.height;
            bottomHeight = originalSize.height - (capInsets.origin.y + centerHeight) - offsets.w;
        }

        if(leftWidth < 0)
        {
            centerWidth += leftWidth;
            leftWidth = 0;
        }
        if(rightWidth < 0)
        {
            centerWidth += rightWidth;
            rightWidth = 0;
        }

        if(topHeight < 0)
        {
            centerHeight += topHeight;
            topHeight = 0;
        }
        if(bottomHeight < 0)
        {
            centerHeight += bottomHeight;
            bottomHeight = 0;
        }

        auto textureRect = CC_RECT_POINTS_TO_PIXELS(_spriteRect);
        //handle .9.png
        if (_isPatch9)
        {
            //This magic number is used to avoiding artifact with .9.png format.
            float offset = 1.3f;
            textureRect = Rect(textureRect.origin.x +  offset,
                               textureRect.origin.y +  offset,
                               textureRect.size.width - 2,
                               textureRect.size.height - 2);
        }

        //uv computation should take spritesheet into account.
        float u0, u1, u2, u3;
        float v0, v1, v2, v3;
        if (_spriteFrameRotated)
        {
            u0 = textureRect.origin.x / atlasWidth;
            u1 = (leftWidth + textureRect.origin.x) / atlasWidth;
            u2 = (leftWidth + centerWidth + textureRect.origin.x) / atlasWidth;
            u3 = (textureRect.origin.x + textureRect.size.height) / atlasWidth;

            v3 = textureRect.origin.y / atlasHeight;
            v2 = (topHeight + textureRect.origin.y) / atlasHeight;
            v1 = (topHeight + centerHeight + textureRect.origin.y) / atlasHeight;
            v0 = (textureRect.origin.y + textureRect.size.width) / atlasHeight;
        }
        else
        {
            u0 = textureRect.origin.x / atlasWidth;
            u1 = (leftWidth + textureRect.origin.x) / atlasWidth;
            u2 = (leftWidth + centerWidth + textureRect.origin.x) / atlasWidth;
            u3 = (textureRect.origin.x + textureRect.size.width) / atlasWidth;

            v0 = textureRect.origin.y / atlasHeight;
            v1 = (topHeight + textureRect.origin.y) / atlasHeight;
            v2 = (topHeight + centerHeight + textureRect.origin.y) / atlasHeight;
            v3 = (textureRect.origin.y + textureRect.size.height) / atlasHeight;
        }


        std::vector<Vec2> uvCoordinates;
        if (_renderingType == RenderingType::SIMPLE)
        {
            uvCoordinates = {Vec2(u0,v3), Vec2(u3,v0)};
        }
        else
        {
            uvCoordinates = {Vec2(u0,v3), Vec2(u1,v2), Vec2(u2,v1), Vec2(u3,v0)};
        }

        return uvCoordinates;
    }

    //
    // y3----------------------(preferedSize.width, preferedSize.height)
    // |        |      |      |
    // |        |      |      |
    // y2-------O------+------|
    // |        |      |      |
    // |        |      |      |
    // y1-------+------+------|
    // |        |      |      |
    // |        |      |      |
    //x0,y0--------------------
    //         x1     x2     x3
    std::vector<Vec2> Scale9Sprite::calculateVertices(const Rect& capInsets,
                                                     const Size& originalSize,
                                                     const Vec4& offsets)
    {

        float offsetLeft = offsets.x / CC_CONTENT_SCALE_FACTOR();
        float offsetTop = offsets.y / CC_CONTENT_SCALE_FACTOR();
        float offsetRight = offsets.z / CC_CONTENT_SCALE_FACTOR();
        float offsetBottom = offsets.w / CC_CONTENT_SCALE_FACTOR();

        std::vector<Vec2> vertices;
        if (_renderingType == RenderingType::SIMPLE)
        {
            float hScale = _preferredSize.width / (originalSize.width / CC_CONTENT_SCALE_FACTOR());
            float vScale = _preferredSize.height / (originalSize.height / CC_CONTENT_SCALE_FACTOR());

            vertices = {Vec2(offsetLeft * hScale, offsetBottom * vScale),
                        Vec2(_preferredSize.width - offsetRight * hScale, _preferredSize.height - offsetTop * vScale)};
        }
        else
        {
            float leftWidth = 0, centerWidth = 0, rightWidth = 0;
            float topHeight = 0, centerHeight = 0, bottomHeight = 0;

            leftWidth = capInsets.origin.x;
            centerWidth = capInsets.size.width;
            rightWidth = originalSize.width - (leftWidth + centerWidth);

            topHeight = capInsets.origin.y;
            centerHeight = capInsets.size.height;
            bottomHeight = originalSize.height - (topHeight + centerHeight);

            leftWidth = leftWidth / CC_CONTENT_SCALE_FACTOR();
            rightWidth = rightWidth / CC_CONTENT_SCALE_FACTOR();
            centerWidth = centerWidth / CC_CONTENT_SCALE_FACTOR();
            topHeight = topHeight / CC_CONTENT_SCALE_FACTOR();
            bottomHeight = bottomHeight / CC_CONTENT_SCALE_FACTOR();
            centerHeight = centerHeight / CC_CONTENT_SCALE_FACTOR();

            float sizableWidth = _preferredSize.width - leftWidth - rightWidth;
            float sizableHeight = _preferredSize.height - topHeight - bottomHeight;

            leftWidth -= offsetLeft;
            rightWidth -= offsetRight;
            topHeight -= offsetTop;
            bottomHeight -= offsetBottom;

            float hScale = sizableWidth / centerWidth;
            float vScale = sizableHeight / centerHeight;

            if(leftWidth<0)
            {
                offsetLeft -= leftWidth * (hScale - 1.0f);
                sizableWidth += leftWidth * hScale;
                leftWidth = 0;
            }
            if(rightWidth<0)
            {
                sizableWidth += rightWidth * hScale;
                rightWidth = 0;
            }
            if(topHeight<0)
            {
                sizableHeight += topHeight * vScale;
                topHeight = 0;
            }
            if(bottomHeight<0)
            {
                offsetBottom -= bottomHeight * (vScale - 1.0f);
                sizableHeight += bottomHeight * vScale;
                bottomHeight = 0;
            }

            float x0,x1,x2,x3;
            float y0,y1,y2,y3;
            if(sizableWidth >= 0)
            {
                x0 = offsetLeft;
                x1 = x0 + leftWidth;
                x2 = x1 + sizableWidth;
                x3 = x2 + rightWidth;
            }
            else
            {
                float xScale = _preferredSize.width / (leftWidth + rightWidth);
                x0 = offsetLeft;
                x1 = x2 = offsetLeft + leftWidth * xScale;
                x3 = x2 + rightWidth * xScale;
            }

            if(sizableHeight >= 0)
            {
                y0 = offsetBottom;
                y1 = y0 + bottomHeight;
                y2 = y1 + sizableHeight;
                y3 = y2 + topHeight;
            }
            else
            {
                float yScale = _preferredSize.height / (topHeight + bottomHeight);
                y0 = offsetBottom;
                y1 = y2 = y0 + bottomHeight * yScale;
                y3 = y2 + topHeight * yScale;
            }

            vertices = {Vec2(x0,y0), Vec2(x1,y1), Vec2(x2,y2), Vec2(x3,y3)};
        }
        return vertices;
    }

    Triangles Scale9Sprite::calculateTriangles(const std::vector<Vec2>& uv,
                                                                const std::vector<Vec2>& vertices)
    {
        const unsigned short slicedTotalVertexCount = powf(uv.size(),2);
        const unsigned short slicedTotalIndices = 6 * powf(uv.size() -1, 2);
        CC_SAFE_DELETE_ARRAY(_sliceVertices);
        CC_SAFE_DELETE_ARRAY(_sliceIndices);

        _sliceVertices = new (std::nothrow) V3F_C4B_T2F[slicedTotalVertexCount];
        _sliceIndices = new (std::nothrow) unsigned short[slicedTotalIndices];

        unsigned short indicesStart = 0;
        const unsigned short indicesOffset = 6;
        const unsigned short sliceQuadIndices[] = {4,0,5, 1,5,0};
        const unsigned short simpleQuadIndices[] = {0,1,2, 3,2,1};

        auto displayedColor = _scale9Image->getDisplayedColor();
        auto displayedOpacity = _scale9Image->getDisplayedOpacity();
        Color4B color4( displayedColor.r, displayedColor.g, displayedColor.b, displayedOpacity );

        // special opacity for premultiplied textures
        if (_scale9Image->isOpacityModifyRGB())
        {
            color4.r *= displayedOpacity/255.0f;
            color4.g *= displayedOpacity/255.0f;
            color4.b *= displayedOpacity/255.0f;
        }

        int vertexCount = (int)(vertices.size() - 1);

        for (int j = 0; j <= vertexCount; ++j)
        {
            for (int i = 0; i <= vertexCount; ++i)
            {
                V3F_C4B_T2F vertextData;
                vertextData.vertices.x = vertices[i].x;
                vertextData.vertices.y = vertices[j].y;

                if (_spriteFrameRotated)
                {
                    vertextData.texCoords.u = uv[j].x;
                    vertextData.texCoords.v = uv[i].y;
                }
                else
                {
                    vertextData.texCoords.u = uv[i].x;
                    vertextData.texCoords.v = uv[j].y;
                }

                vertextData.colors = color4;

                //if slice mode
                if (_renderingType == RenderingType::SLICE)
                {
                    memcpy(_sliceVertices + i + j * 4, &vertextData, sizeof(V3F_C4B_T2F));
                }
                else
                {
                    memcpy(_sliceVertices + i + j * 2, &vertextData, sizeof(V3F_C4B_T2F));
                }
            }
        }

        if (_renderingType == RenderingType::SLICE)
        {
            for (int j = 0; j <= vertexCount; ++j)
            {
                for (int i = 0; i <= vertexCount; ++i)
                {
                    if (i < 3 && j < 3)
                    {
                        memcpy(_sliceIndices + indicesStart, sliceQuadIndices, indicesOffset * sizeof(unsigned short));

                        for (int k = 0; k  < indicesOffset; ++k)
                        {
                            unsigned short actualIndex = (i  + j * 3) * indicesOffset;
                            _sliceIndices[k + actualIndex] = _sliceIndices[k + actualIndex] + j * 4 + i;
                        }
                        indicesStart = indicesStart + indicesOffset;
                    }

                }
            }
        }

        if (_renderingType == RenderingType::SIMPLE)
        {
            memcpy(_sliceIndices, simpleQuadIndices, indicesOffset * sizeof(unsigned short));
        }

        Triangles triangles;
        triangles.vertCount = slicedTotalVertexCount;
        triangles.indexCount = slicedTotalIndices;
        triangles.verts = _sliceVertices;
        triangles.indices = _sliceIndices;

        return triangles;
    }

    void Scale9Sprite::setRenderingType(cocos2d::ui::Scale9Sprite::RenderingType type)
    {
        if (_renderingType == type)
        {
            return;
        }
        _renderingType = type;
        _sliceSpriteDirty = true;
    }

    Scale9Sprite::RenderingType Scale9Sprite::getRenderingType()const
    {
        return _renderingType;
    }

    void Scale9Sprite::resetRender()
    {
        // Release old sprites
        this->cleanupSlicedSprites();

        CC_SAFE_RELEASE_NULL(this->_scale9Image);
    }

    void Scale9Sprite::setGlobalZOrder(float globalZOrder)
    {
        Node::setGlobalZOrder(globalZOrder);
        if (_scale9Image)
        {
            _scale9Image->setGlobalZOrder(globalZOrder);
        }
    }

}}
