/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CONSTS_H_INCL__
#define __CONSTS_H_INCL__

//---------------------------------------------------------------------------
// Error Codes
//---------------------------------------------------------------------------

#define ERR_OK                 0   /**< Operation completed successfully */
#define ERR_FOPEN            101   /**< Failed to open file */
#define ERR_FEMPTY           102   /**< File is empty */
#define ERR_INV_LAND         103   /**< Invalid land/hex specification */
#define ERR_LOGIC            104   /**< Internal logic error */
#define ERR_DUPLICATE_UNIT   105   /**< Duplicate unit ID detected */
#define ERR_FNAME            106   /**< Invalid filename */
#define ERR_CANCEL           107   /**< Operation cancelled by user */
#define ERR_NOTHING          108   /**< No data was parsed */
#define ERR_INV_TURN         109   /**< Joining report for wrong turn */
#define ERR_INV_UNIT         110   /**< Invalid unit specification */

//---------------------------------------------------------------------------
// Report Headers
//---------------------------------------------------------------------------

#define HDR_FACTION          "Atlantis Report For:"    /**< Faction header in report */
#define HDR_FACTION_STATUS   "Faction Status:"         /**< Faction status header */
#define HDR_ERRORS           "Errors during turn:"     /**< Errors section header */
#define HDR_EVENTS           "Events during turn:"     /**< Events section header */
#define HDR_EVENTS_2         "Important Messages:"     /**< Important messages header */
#define HDR_SILVER           "Unclaimed silver:"       /**< Unclaimed silver header */
#define HDR_SKILLS           "Skill reports:"          /**< Skills section header */
#define HDR_ITEMS            "Item reports:"           /**< Items section header */
#define HDR_OBJECTS          "Object reports:"         /**< Objects section header */
#define HDR_BATTLES          "Battles during turn:"    /**< Battles section header */
#define HDR_ATTACKERS        "Attackers:"              /**< Attackers in battle header */
#define HDR_DEFENDERS        "Defenders:"              /**< Defenders in battle header */
#define HDR_CASUALTIES       "Total Casualties:"       /**< Casualties header */
#define HDR_SPOILS           "Spoils:"                 /**< Battle spoils header */
#define HDR_NOSPOILS         "No spoils."              /**< No spoils message */
#define HDR_ATTITUDES        "Declared Attitudes ("    /**< Attitudes section header (opening) */
#define HDR_ORDER_TEMPLATE   "Orders Template ("       /**< Orders template header (opening) */

//---------------------------------------------------------------------------
// Unit Display Prefixes
//---------------------------------------------------------------------------

#define HDR_UNIT_OWN         "* "                       /**< Prefix for own faction units */
#define HDR_UNIT_ALIEN       "- "                       /**< Prefix for alien faction units */
#define HDR_STRUCTURE        "+ "                       /**< Prefix for structure entries */

//---------------------------------------------------------------------------
// Game Rules Constants
//---------------------------------------------------------------------------

/**
 * @def STUDENTS_PER_TEACHER
 * @brief Maximum number of students per teacher
 * 
 * Standard Atlantis rule: one teacher can teach up to 10 students
 * in a single turn.
 */
#define STUDENTS_PER_TEACHER 10

#endif