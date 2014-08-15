/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014 Vaclav Slavik
 *
 *  All rights reserved.
 *
 *  This source file is *not* under the Open Source license that covers
 *  the Open Source version of Poedit.
 *
 */


// Xcode has issues with signing plugins/bundles that don't actually have
// a binary in them. To work around this limitation, create a tiny dummy
// plugin binary with a dummy function in it.
int lets_keep_xcode_codesigning_happy_la_la_la()
{
    return 42;
}