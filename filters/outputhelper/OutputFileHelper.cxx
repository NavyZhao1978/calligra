/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* writerperfect
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2007 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwpd.sourceforge.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "OutputFileHelper.hxx"

#ifdef USE_GSF_OUTPUT
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#include <gsf/gsf-input-stdio.h>
#else
#include "FemtoZip.hxx"
#endif

struct OutputFileHelperImpl
{
	OutputFileHelperImpl(const char *password) : mpOutfile(0), mpPassword(password) {}
#ifdef USE_GSF_OUTPUT
	GsfOutfile *mpOutfile;
#else
	FemtoZip *mpOutfile;
#endif
	const char *mpPassword;
private:
	OutputFileHelperImpl(OutputFileHelperImpl const &);
	OutputFileHelperImpl &operator=(OutputFileHelperImpl const &);
};


OutputFileHelper::OutputFileHelper(const char *outFileName, const char *password) :
#ifdef USE_GSF_OUTPUT
	m_impl(new OutputFileHelperImpl(password))
#else
	m_impl(new OutputFileHelperImpl(password))
#endif
{
	m_impl->mpOutfile = 0;
#ifdef USE_GSF_OUTPUT
	GsfOutput  *pOutput = 0;
	GError   *err = 0;

	gsf_init();

	if (!outFileName)
		pOutput = 0;
	else
	{
		pOutput = GSF_OUTPUT(gsf_output_stdio_new(outFileName, &err));
		if (pOutput == 0)
		{
			if (err)
			{
				g_warning("'%s' error: %s", outFileName, err->message);
				g_error_free(err);
			}
			gsf_shutdown();
		}
		else
		{
			if (err)
				g_error_free(err);
			err = 0;
			m_impl->mpOutfile = GSF_OUTFILE(gsf_outfile_zip_new(pOutput, &err));
			if (m_impl->mpOutfile == 0)
			{
				if (err)
				{
					g_warning("'%s' error: %s",
					          "gsf_outfile_zip_new", err->message);
					g_error_free(err);
				}
				gsf_shutdown();
			}
			else
			{
				if (err)
					g_error_free(err);
				err = 0;
				g_object_unref(pOutput);
			}
		}
	}
#else
	if (outFileName)
		m_impl->mpOutfile = new FemtoZip(outFileName);
#endif
}

OutputFileHelper::~OutputFileHelper()
{
#ifdef USE_GSF_OUTPUT
	if (m_impl->mpOutfile && !gsf_output_close((GsfOutput *) m_impl->mpOutfile))
		fprintf(stderr, "ERROR : Couldn't close outfile\n");

	if (m_impl->mpOutfile)
		g_object_unref(m_impl->mpOutfile);

	gsf_shutdown();
#else
	if (m_impl->mpOutfile)
		delete m_impl->mpOutfile;
#endif
	if (m_impl)
		delete m_impl;
}

bool OutputFileHelper::writeChildFile(const char *childFileName, const char *str)
{
	if (!m_impl->mpOutfile)
		return true;
#ifdef USE_GSF_OUTPUT
	GsfOutput *child;
	if (0 != (child = gsf_outfile_new_child(m_impl->mpOutfile, childFileName, FALSE)))
	{
		bool res = gsf_output_puts(child, str) &&
		           gsf_output_close(child);
		g_object_unref(child);
		return res;
	}
	return false;
#else
	m_impl->mpOutfile->createEntry(childFileName, 0);
	if (m_impl->mpOutfile->errorCode())
		return false;
	m_impl->mpOutfile->writeString(str);
	if (m_impl->mpOutfile->errorCode())
		return false;
	m_impl->mpOutfile->closeEntry();
	if (m_impl->mpOutfile->errorCode())
		return false;
	return true;
#endif
}

#if defined(USE_GSF_OUTPUT) && defined(GSF_HAS_COMPRESSION_LEVEL)
bool OutputFileHelper::writeChildFile(const char *childFileName, const char *str, const char compression_level)
#else
bool OutputFileHelper::writeChildFile(const char *childFileName, const char *str, const char)
#endif
{
	if (!m_impl->mpOutfile)
		return true;
#ifdef USE_GSF_OUTPUT
	GsfOutput *child;
#ifdef GSF_HAS_COMPRESSION_LEVEL
	if (0 != (child = gsf_outfile_new_child_full(m_impl->mpOutfile, childFileName, FALSE,"compression-level", compression_level, (void *)0)))
#else
	if (0 != (child = gsf_outfile_new_child(m_impl->mpOutfile, childFileName, FALSE)))
#endif
	{
		bool res = gsf_output_puts(child, str) &&
		           gsf_output_close(child);
		g_object_unref(child);
		return res;
	}
	return false;
#else
	m_impl->mpOutfile->createEntry(childFileName, 0); // only storing without compressing works with FemtoZip
	if (m_impl->mpOutfile->errorCode())
		return false;
	m_impl->mpOutfile->writeString(str);
	if (m_impl->mpOutfile->errorCode())
		return false;
	m_impl->mpOutfile->closeEntry();
	if (m_impl->mpOutfile->errorCode())
		return false;
	return true;
#endif
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
