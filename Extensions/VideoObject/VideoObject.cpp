/**

Game Develop - Video Object Extension
Copyright (c) 2010 Florian Rival (Florian.Rival@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.

*/

#include <SFML/Graphics.hpp>
#include "GDL/Object.h"

#include "GDL/ImageManager.h"
#include "GDL/tinyxml.h"
#include "GDL/FontManager.h"
#include "GDL/Position.h"
#include "GDL/XmlMacros.h"
#include "VideoObject.h"

#ifdef GDE
#include <wx/wx.h>
#include "GDL/CommonTools.h"
#include "GDL/ResourcesMergingHelper.h"
#include "GDL/MainEditorCommand.h"
#include "VideoObjectEditor.h"
#endif

VideoObject::VideoObject(std::string name_) :
    Object(name_),
    looping(true),
    paused(false),
    opacity( 255 ),
    colorR( 255 ),
    colorG( 255 ),
    colorB( 255 ),
    angle(0)
{
}

VideoObject::~VideoObject()
{
}

void VideoObject::LoadFromXml(const TiXmlElement * elem)
{
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("videoFile", videoFile);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_BOOL("looping", looping);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_FLOAT("opacity", opacity);

    int r = 255;
    int g = 255;
    int b = 255;
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_INT("colorR", r);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_INT("colorG", g);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_INT("colorB", b);
    SetColor(r,g,b);
}

#if defined(GDE)
void VideoObject::SaveToXml(TiXmlElement * elem)
{
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("videoFile", videoFile);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_BOOL("looping", looping);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_FLOAT("opacity", opacity);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE("colorR", colorR);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE("colorG", colorG);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE("colorB", colorB);
}
#endif

bool VideoObject::LoadRuntimeResources(const ImageManager & imageMgr )
{
    ReloadVideo();
    return true;
}

void VideoObject::ReloadVideo()
{
    video.Load(videoFile);
    video.SetLooping(looping);
    video.SetPause(paused);
    renderSprite.SetImage(video.GetNextFrameImage(), true);
    renderSprite.SetOrigin(renderSprite.GetSize().x/2, renderSprite.GetSize().y/2);
}

/**
 * Update animation and direction from the inital position
 */
bool VideoObject::InitializeFromInitialPosition(const InitialPosition & position)
{
    return true;
}

/**
 * Render object at runtime
 */
bool VideoObject::Draw( sf::RenderWindow& window )
{
    //Don't draw anything if hidden
    if ( hidden ) return true;

	renderSprite.SetImage(video.GetNextFrameImage(), true);
    window.Draw( renderSprite );

    return true;
}

#ifdef GDE
/**
 * Render object at edittime
 */
bool VideoObject::DrawEdittime(sf::RenderWindow& renderWindow)
{
    if ( !video.IsValid() )
    {
        static bool badImageLoaded = false;
        static sf::Image badVideoIcon;
        if ( !badImageLoaded )
        {
            badVideoIcon.LoadFromFile("Extensions/badVideo.png");
            badImageLoaded = true;
        }
        renderSprite.SetImage(badVideoIcon, true);
    }
    else
        renderSprite.SetImage(video.GetNextFrameImage(), true);

    renderWindow.Draw( renderSprite );

    return true;
}

void VideoObject::PrepareResourcesForMerging(ResourcesMergingHelper & resourcesMergingHelper)
{
}

bool VideoObject::GenerateThumbnail(const Game & game, wxBitmap & thumbnail)
{
    thumbnail = wxBitmap("Extensions/videoicon24.png", wxBITMAP_TYPE_ANY);

    return true;
}

void VideoObject::EditObject( wxWindow* parent, Game & game, MainEditorCommand & mainEditorCommand )
{
    VideoObjectEditor dialog(parent, game, *this, mainEditorCommand);
    dialog.ShowModal();
}

wxPanel * VideoObject::CreateInitialPositionPanel( wxWindow* parent, const Game & game_, const Scene & scene_, const InitialPosition & position )
{
    return NULL;
}

void VideoObject::UpdateInitialPositionFromPanel(wxPanel * panel, InitialPosition & position)
{
}

void VideoObject::GetPropertyForDebugger(unsigned int propertyNb, string & name, string & value) const
{
    if      ( propertyNb == 0 ) {name = _("Fichier");                     value = GetVideoFile();}
    else if ( propertyNb == 1 ) {name = _("Bouclage");                    value = looping ? _("Oui") : _("Non");}
    else if ( propertyNb == 2 ) {name = _("Position dans la vid�o");      value = ToString(video.GetTimePosition())+"s";}
    else if ( propertyNb == 3 ) {name = _("En pause");                    value = paused ? _("Oui") : _("Non");}
    else if ( propertyNb == 4 ) {name = _("Dur�e");                       value = ToString(video.GetDuration())+"s";}
    else if ( propertyNb == 5 ) {name = _("Couleur");                     value = ToString(colorR)+";"+ToString(colorG)+";"+ToString(colorB);}
    else if ( propertyNb == 6 ) {name = _("Opacit�");       value = ToString(GetOpacity());}
}

bool VideoObject::ChangeProperty(unsigned int propertyNb, string newValue)
{
    if      ( propertyNb == 0 ) { SetVideoFile(newValue); ReloadVideo(); }
    else if ( propertyNb == 1 ) { looping = (newValue == _("Oui")); video.SetLooping(looping); }
    else if ( propertyNb == 2 ) { video.Seek(ToFloat(newValue)); }
    else if ( propertyNb == 3 ) { paused = (newValue == _("Oui")); video.SetPause(paused); }
    else if ( propertyNb == 4 ) { return false; }
    else if ( propertyNb == 5 )
    {
        string r, gb, g, b;
        {
            size_t separationPos = newValue.find(";");

            if ( separationPos > newValue.length())
                return false;

            r = newValue.substr(0, separationPos);
            gb = newValue.substr(separationPos+1, newValue.length());
        }

        {
            size_t separationPos = gb.find(";");

            if ( separationPos > gb.length())
                return false;

            g = gb.substr(0, separationPos);
            b = gb.substr(separationPos+1, gb.length());
        }

        SetColor(ToInt(r), ToInt(g), ToInt(b));
    }
    else if ( propertyNb == 6 ) { SetOpacity(ToFloat(newValue)); }

    return true;
}

unsigned int VideoObject::GetNumberOfProperties() const
{
    return 7;
}
#endif

void VideoObject::OnPositionChanged()
{
    renderSprite.SetX( GetX() );
    renderSprite.SetY( GetY() );
}

/**
 * Get the real X position of the sprite
 */
float VideoObject::GetDrawableX() const
{
    return renderSprite.GetPosition().x-renderSprite.GetOrigin().x;
}

/**
 * Get the real Y position of the text
 */
float VideoObject::GetDrawableY() const
{
    return renderSprite.GetPosition().y-renderSprite.GetOrigin().y;
}

/**
 * Width is the width of the current sprite.
 */
float VideoObject::GetWidth() const
{
    return renderSprite.GetSize().x;
}

/**
 * Height is the height of the current sprite.
 */
float VideoObject::GetHeight() const
{
    return renderSprite.GetSize().y;
}

/**
 * X center is computed with text rectangle
 */
float VideoObject::GetCenterX() const
{
    return renderSprite.GetSize().x/2;
}

/**
 * Y center is computed with text rectangle
 */
float VideoObject::GetCenterY() const
{
    return renderSprite.GetSize().y/2;
}

/**
 * Nothing to do when updating time
 */
void VideoObject::UpdateTime(float time_increase)
{
    video.UpdateTime(time_increase);
}

/**
 * Change the color filter of the sprite object
 */
void VideoObject::SetColor( unsigned int r, unsigned int g, unsigned int b )
{
    colorR = r;
    colorG = g;
    colorB = b;
    renderSprite.SetColor(sf::Color(colorR, colorG, colorB, opacity));
}

void VideoObject::SetOpacity(float val)
{
    if ( val > 255 )
        val = 255;
    else if ( val < 0 )
        val = 0;

    opacity = val;
    renderSprite.SetColor(sf::Color(colorR, colorG, colorB, opacity));
}

/**
 * Function destroying an extension Object.
 * Game Develop does not delete directly extension object
 * to avoid overloaded new/delete conflicts.
 */
void DestroyVideoObject(Object * object)
{
    delete object;
}

/**
 * Function creating an extension Object.
 * Game Develop can not directly create an extension object
 */
Object * CreateVideoObject(std::string name)
{
    return new VideoObject(name);
}
