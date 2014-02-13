/* **************************************************************************************************
* GestureToMenuApp.cpp -- is an app which takes in the hand gestures as an input and appropriate 
* image files are loaded on to the display as a result. However it will be modified to include more than 
* one image at a time be present on the display and finger pointers will be included on the images selected
* ******************************************************************************************************* */

//cinder header files used
#include "cinder/app/AppBasic.h"
#include "cinder/app/AppNative.h"
#include "cinder/app/App.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

//#include "cinder/gl/gl.h"
#include "cinder/Filesystem.h"
#include "cinder/params/Params.h"
#include "Cinder-LeapMotion.h"

//#include "Leap.h"
#include "cinder/Thread.h"
#include "cinder/Vector.h"
#include <functional>
#include "Resources.h"

//namespaces used
using namespace ci;
using namespace ci::app;
using namespace std;
using namespace LeapMotion;
/////////////////////////////////////////////////////////////////////////////////////////////////
class GestureToMenuApp : public ci::app::AppBasic
{
public:
	void					draw();
	void					prepareSettings( ci::app::AppBasic::Settings* settings );
	void					resize();
	void					setup();
	void					update();
private:
	// Leap
	Leap::Frame				mFrame;
	LeapMotion::DeviceRef	mDevice;
	void 					onFrame( Leap::Frame frame );
	ci::Vec2f				warpPointable( const Leap::Pointable& p );
	ci::Vec2f				warpVector( const Leap::Vector& v );

	//Image file
	gl::Texture				mImage;
	
	// UI
	float					mBackgroundBrightness;
	ci::Colorf				mBackgroundColor;
	int32_t					mCircleResolution;
	float					mDialBrightness;
	ci::Vec2f				mDialPosition;
	float					mDialRadius;
	float					mDialSpeed;
	float					mDialValue;
	float					mDialValueDest;
	float					mDotRadius;
	float					mDotSpacing;
	float					mFadeSpeed;
	float					mKeySpacing;
	ci::Rectf				mKeyRect;
	float					mKeySize;
	ci::Vec2f				mOffset;
	float					mPointableRadius;
	float					mSwipeBrightness;
	float					mSwipePos;
	float					mSwipePosDest;
	float					mSwipePosSpeed;
	ci::Rectf				mSwipeRect;
	float					mSwipeStep;
	
	// A key to press
	struct Key
	{
		Key( const ci::Rectf& bounds = ci::Rectf() )
		: mBounds( bounds ), mBrightness( 0.0f )
		{
		}
		ci::Rectf			mBounds;
		float				mBrightness;
	};
	std::vector<Key>		mKeys;

	// Rendering
	void					drawDottedCircle( const ci::Vec2f& center, float radius,
											 float dotRadius, int32_t resolution,
											 float progress = 1.0f );
	void					drawDottedRect( const ci::Vec2f& center, const ci::Vec2f& size );
	void					drawGestures();
	void					drawPointables();
	void					drawUi();
	
	// Params
	float					mFrameRate;
	bool					mFullScreen;
	ci::params::InterfaceGl	mParams;
	
	// Save screen shot
	//void					screenShot();
};

// Render
void GestureToMenuApp::draw()
{
	// Clear window
	gl::setViewport( getWindowBounds() );
	gl::clear( mBackgroundColor + Colorf::gray( mBackgroundBrightness ) );
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::color( Colorf::white() );
	
	// Draw everything
	gl::pushMatrices();
	gl::translate( mOffset );
	
	drawUi();
	drawGestures();
	drawPointables();
	
	gl::popMatrices();
	
	// Draw the interface
	mParams.draw();

	//draw the image loaded based on the Gesture 
	if (mImage)
	gl::draw(mImage, getWindowBounds());

}

// Draw dotted circle
void GestureToMenuApp::drawDottedCircle( const Vec2f& center, float radius, float dotRadius,
								  int32_t resolution, float progress )
{
	float twoPi		= (float)M_PI * 2.0f;
	progress		*= twoPi;
	float delta		= twoPi / (float)resolution;
	for ( float theta = 0.0f; theta <= progress; theta += delta ) {
		float t		= theta - (float)M_PI * 0.5f;
		float x		= math<float>::cos( t );
		float y		= math<float>::sin( t );
		
		Vec2f pos	= center + Vec2f( x, y ) * radius;
		gl::drawSolidCircle( pos, dotRadius, 32 );
	}
}

// Draw dotted rectangle
void GestureToMenuApp::drawDottedRect( const Vec2f& center, const Vec2f &size )
{
	Rectf rect( center - size, center + size );
	Vec2f pos = rect.getUpperLeft();
	while ( pos.x < rect.getX2() ) {
		gl::drawSolidCircle( pos, mDotRadius, mCircleResolution );
		pos.x += mDotSpacing;
	}
	while ( pos.y < rect.getY2() ) {
		gl::drawSolidCircle( pos, mDotRadius, mCircleResolution );
		pos.y += mDotSpacing;
	}
	while ( pos.x > rect.getX1() ) {
		gl::drawSolidCircle( pos, mDotRadius, mCircleResolution );
		pos.x -= mDotSpacing;
	}
	while ( pos.y > rect.getY1() ) {
		gl::drawSolidCircle( pos, mDotRadius, mCircleResolution );
		pos.y -= mDotSpacing;
	}
}

// Draw active gestures with dotted lines
void GestureToMenuApp::drawGestures()
{
	gl::color( ColorAf::white() );
	
	// Iterate through gestures
	const Leap::GestureList& gestures = mFrame.gestures();
	for ( Leap::GestureList::const_iterator iter = gestures.begin(); iter != gestures.end(); ++iter ) {
		const Leap::Gesture& gesture	= *iter;
		Leap::Gesture::Type type		= gesture.type();
		if ( type == Leap::Gesture::Type::TYPE_CIRCLE ) {
			
			// Cast to circle gesture and read data
			const Leap::CircleGesture& gesture = (Leap::CircleGesture)*iter;
						
			Vec2f pos	= warpVector( gesture.center() );
			float progress	= gesture.progress();
			float radius	= gesture.radius() * 2.0f; // Don't ask, it works
			
			//Hopefully a dotted circle is drawn on the top of the image
			drawDottedCircle( pos, radius, mDotRadius, mCircleResolution, progress );

			//Load appropriate resource based on a particular Gesture
			mImage = gl::Texture( loadImage(loadResource("Menu_130.png" , 130, "IMAGE")));

		} else if ( type == Leap::Gesture::Type::TYPE_KEY_TAP ) {
			
			// Cast to circle gesture and read data
			const Leap::KeyTapGesture& gesture = (Leap::KeyTapGesture)*iter;
			Vec2f center = warpVector( gesture.position() );

			mImage = gl::Texture( loadImage(loadResource("Menu_131.png", 131, "IMAGE")));
			// Draw square where key press happened
			Vec2f size( 30.0f, 30.0f );
			drawDottedRect( center, size );
			
			

		} else if ( type == Leap::Gesture::Type::TYPE_SCREEN_TAP ) {
			
			// Draw big square on center of screen
		
			Vec2f center = getWindowCenter();
			Vec2f size( 300.0f, 300.0f );

			mImage = gl::Texture( loadImage(loadResource("Menu_129.png", 129, "IMAGE")));

			drawDottedRect( center, size );
		
			

		} else if ( type == Leap::Gesture::Type::TYPE_SWIPE ) {
			
			
			// Cast to swipe gesture and read data
			const Leap::SwipeGesture& gesture = (Leap::SwipeGesture)*iter;
			ci::Vec2f a	= warpVector( gesture.startPosition() );
			ci::Vec2f b	= warpVector( gesture.position() );
			
			mImage = gl::Texture( loadImage(loadResource("Menu_132.png", 132, "IMAGE")));

			// Set draw direction
			float spacing = mDotRadius * 3.0f;
			float direction = 1.0f;
			if ( b.x < a.x ) {
				direction *= -1.0f;
				swap( a, b );
				
			}
		
			// Draw swipe line
			Vec2f pos = a;
			while ( pos.x <= b.x ) {
				pos.x += spacing;
				gl::drawSolidCircle( pos, mDotRadius, 32 );
			}
		
			// Draw arrow head
			if ( direction > 0.0f ) {
				pos		= b;
				spacing	*= -1.0f;
			} else {
				pos		= a;
				pos.x	+= spacing;
			}
			pos.y		= a.y;
			pos.x		+= spacing;
			gl::drawSolidCircle( pos + Vec2f( 0.0f, spacing ), mDotRadius, 32 );
			gl::drawSolidCircle( pos + Vec2f( 0.0f, spacing * -1.0f ), mDotRadius, 32 );
			pos.x		+= spacing;
			gl::drawSolidCircle( pos + Vec2f( 0.0f, spacing * 2.0f ), mDotRadius, 32 );
			gl::drawSolidCircle( pos + Vec2f( 0.0f, spacing * -2.0f ), mDotRadius, 32 );
		
		}

	//mImage = gl::Texture( loadImage(loadResource("cinder_app_icon.Icon", 128, "IMAGE")));

	}
}

// Draw pointables calibrated to the screen
void GestureToMenuApp::drawPointables()
{
	gl::color( ColorAf::white() );
	const Leap::HandList& hands = mFrame.hands();
	for ( Leap::HandList::const_iterator handIter = hands.begin(); handIter != hands.end(); ++handIter ) {
		const Leap::Hand& hand = *handIter;
		const Leap::PointableList& pointables = hand.pointables();
		for ( Leap::PointableList::const_iterator pointIter = pointables.begin(); pointIter != pointables.end(); ++pointIter ) {
			const Leap::Pointable& pointable = *pointIter;
			
			Vec2f pos( warpPointable( pointable ) );
			drawDottedCircle( pos, mPointableRadius, mDotRadius * 0.5f, mCircleResolution / 2 );
		}
	}
}

// Draw interface

void GestureToMenuApp::drawUi()
{
	// Draw dial
	//No dial is not needed 
	
	// Draw swipe
	float x = mSwipeRect.x1 + mSwipePos * mSwipeRect.getWidth();
	Rectf a( mSwipeRect.x1, mSwipeRect.y1, x, mSwipeRect.y2 );
	Rectf b( x, mSwipeRect.y1, mSwipeRect.x2, mSwipeRect.y2 );
	gl::color( ColorAf( Colorf::gray( 0.5f ), 0.2f + mSwipeBrightness ) );
	gl::drawSolidRect( a );
	gl::color( ColorAf( Colorf::white(), 0.2f + mSwipeBrightness ) );
	gl::drawSolidRect( b );
	
	// Draw keys
	// No keys are needed here 
}

// Called when Leap frame data is ready
void GestureToMenuApp::onFrame( Leap::Frame frame )
{
	mFrame = frame;
}

// Prepare window
void GestureToMenuApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 ); //replace 1024, 768
	settings->setFrameRate( 60.0f );
}

// Handles window resize
void GestureToMenuApp::resize()
{
	mOffset = getWindowCenter() - Vec2f::one() * 320.0f;
}

// Set up
void GestureToMenuApp::setup()
{
	// Set up OpenGL
	gl::enable( GL_POLYGON_SMOOTH );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	
	// UI
	mBackgroundBrightness	= 0.0f;
	mBackgroundColor		= Colorf( 0.0f, 0.1f, 0.2f );
	mCircleResolution		= 32;
	mDialBrightness			= 0.0f;
	mDialPosition			= Vec2f( 155.0f, 230.0f );
	mDialRadius				= 120.0f;
	mDialSpeed				= 0.21f;
	mDialValue				= 0.0f;
	mDialValueDest			= mDialValue;
	mDotRadius				= 3.0f;
	mDotSpacing				= mDotRadius * 3.0f;
	mFadeSpeed				= 0.95f;
	mKeySpacing				= 25.0f;
	mKeyRect				= Rectf( mKeySpacing, 360.0f + mKeySpacing, 600.0f, 600.0f );
	mKeySize				= 60.0f;
	mPointableRadius		= 15.0f;
	mSwipeBrightness		= 0.0f;
	mSwipePos				= 0.0f;
	mSwipePosDest			= mSwipePos;
	mSwipePosSpeed			= 0.33f;
	mSwipeRect				= Rectf( 310.0f, 100.0f, 595.0f, 360.0f );
	mSwipeStep				= 0.033f;
	
	// Sets master offset
	resize();
	
	// Lay out keys 
	// this is not needed in GestureToMenuApp
	
	// Start device
	mDevice = Device::create();
	mDevice->connectEventHandler( &GestureToMenuApp::onFrame, this );

	// Enable all gesture types
	Leap::Controller* controller = mDevice->getController();
	controller->enableGesture( Leap::Gesture::Type::TYPE_CIRCLE );
	controller->enableGesture( Leap::Gesture::Type::TYPE_KEY_TAP );
	controller->enableGesture( Leap::Gesture::Type::TYPE_SCREEN_TAP );
	controller->enableGesture( Leap::Gesture::Type::TYPE_SWIPE );
	
	// Write gesture config to console
	Leap::Config config = controller->config();
	
	// Update config to make gestures easier
	config.setFloat( "Gesture.Circle.MinRadius", 2.5f );
	config.setFloat( "Gesture.Circle.MinArc", 3.0f );
	config.setFloat( "Gesture.Swipe.MinLength", 75.0f );
	config.setFloat( "Gesture.Swipe.MinVelocity", 500.0f );
	config.setFloat( "Gesture.KeyTap.MinDownVelocity", 25.0f );
	
	// Allows app to run in background
	controller->setPolicyFlags( Leap::Controller::PolicyFlag::POLICY_BACKGROUND_FRAMES );
	
	// Params
	
	mFrameRate	= 0.0f;
	mFullScreen	= false;
	mParams = params::InterfaceGl( "Params", Vec2i( 200, 105 ) );
	mParams.addParam( "Frame rate",		&mFrameRate,							"", true	);
	mParams.addParam( "Full screen",	&mFullScreen,							"key=f"		);
	//mParams.addButton( "Screen shot",	bind( &GestureToMenuApp::screenShot, this ), "key=space"	);
	mParams.addButton( "Quit",			bind( &GestureToMenuApp::quit, this ),		"key=q"		);

}

// Runs update logic
void GestureToMenuApp::update()
{
	// Update frame rate
	mFrameRate = getAverageFps();

	// Toggle fullscreen
	if ( mFullScreen != isFullScreen() ) {
		setFullScreen( mFullScreen );
	}

	const Leap::GestureList& gestures = mFrame.gestures();
	for ( Leap::GestureList::const_iterator iter = gestures.begin(); iter != gestures.end(); ++iter ) {
		const Leap::Gesture& gesture	= *iter;
		Leap::Gesture::Type type		= gesture.type();
		if ( type == Leap::Gesture::Type::TYPE_CIRCLE ) {
			
			// Cast to circle gesture
			const Leap::CircleGesture& gesture = (Leap::CircleGesture)*iter;
			
			// Control dial
			mDialBrightness	= 1.0f;
			mDialValueDest	= gesture.progress();
			
		} else if ( type == Leap::Gesture::Type::TYPE_KEY_TAP ) {
			
			// Cast to circle gesture and read data
			const Leap::KeyTapGesture& gesture = (Leap::KeyTapGesture)*iter;
			Vec2f center	= warpVector( gesture.position() );
			center			-= mOffset;
			
			// Press key
			for ( vector<Key>::iterator keyIter = mKeys.begin(); keyIter != mKeys.end(); ++keyIter ) {
				if ( keyIter->mBounds.contains( center ) ) {
					keyIter->mBrightness = 1.0f;
					break;
				}
			}
			
		} else if ( type == Leap::Gesture::Type::TYPE_SCREEN_TAP ) {
			
			// Turn background white for screen tap
			mBackgroundBrightness = 1.0f;
			
		} else if ( type == Leap::Gesture::Type::TYPE_SWIPE ) {
			
			// Cast to swipe gesture and read data
			const Leap::SwipeGesture& swipeGesture = (Leap::SwipeGesture)gesture;
			ci::Vec2f a	= warpVector( swipeGesture.startPosition() );
			ci::Vec2f b	= warpVector( swipeGesture.position() );
			
			// Update swipe position
			mSwipeBrightness	= 1.0f;
			if ( gesture.state() == Leap::Gesture::State::STATE_STOP ) {
				mSwipePosDest	= b.x < a.x ? 0.0f : 1.0f;
			} else {
				float step		= mSwipeStep;
				mSwipePosDest	+= b.x < a.x ? -step : step;
			}
			mSwipePosDest		= math<float>::clamp( mSwipePosDest, 0.0f, 1.0f );
		}
	}
	
	// UI animation
	//Not needed in our case to animate 
	mDialValue				= lerp( mDialValue, mDialValueDest, mDialSpeed );
	mSwipePos				= lerp( mSwipePos, mSwipePosDest, mSwipePosSpeed );
	mBackgroundBrightness	*= mFadeSpeed;
	mDialBrightness			*= mFadeSpeed;
	mSwipeBrightness		*= mFadeSpeed;
	for ( vector<Key>::iterator iter = mKeys.begin(); iter != mKeys.end(); ++iter ) {
		iter->mBrightness *= mFadeSpeed;
	}
}


// Maps pointable's ray to the screen in pixels
Vec2f GestureToMenuApp::warpPointable( const Leap::Pointable& p )
{
	Vec3f result	= Vec3f::zero();
	if ( mDevice ) {
		const Leap::Screen& screen = mDevice->getController()->locatedScreens().closestScreenHit( p );
		
		result		= LeapMotion::toVec3f( screen.intersect( p, true, 1.0f ) );
	}
	result			*= Vec3f( Vec2f( getWindowSize() ), 0.0f );
	result.y		= (float)getWindowHeight() - result.y;
	return result.xy();
}

// Maps Leap vector to the screen in pixels
Vec2f GestureToMenuApp::warpVector( const Leap::Vector& v )
{
	Vec3f result	= Vec3f::zero();
	if ( mDevice ) {
		const Leap::Screen& screen = mDevice->getController()->locatedScreens().closestScreen( v );
		
		result		= LeapMotion::toVec3f( screen.project( v, true ) );
	}
	result			*= Vec3f( getWindowSize(), 0.0f );
	result.y		= (float)getWindowHeight() - result.y;
	return result.xy();
}

// Run application
CINDER_APP_BASIC( GestureToMenuApp, RendererGl )
